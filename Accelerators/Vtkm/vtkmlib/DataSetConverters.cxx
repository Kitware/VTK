//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#include "DataSetConverters.h"

#include "ArrayConverters.h"
#include "CellSetConverters.h"
#include "ImageDataConverter.h"
#include "PolyDataConverter.h"
#include "UnstructuredGridConverter.h"

#include "vtkmDataArray.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CoordinateSystem.hxx>
#include <vtkm/cont/Field.h>

namespace tovtkm
{

namespace
{

template <typename T>
vtkm::cont::CoordinateSystem deduce_container(vtkPoints* points)
{
  typedef vtkm::Vec<T, 3> Vec3;

  vtkAOSDataArrayTemplate<T>* typedIn = vtkAOSDataArrayTemplate<T>::FastDownCast(points->GetData());
  if (typedIn)
  {
    auto p = DataArrayToArrayHandle<vtkAOSDataArrayTemplate<T>, 3>::Wrap(typedIn);
    return vtkm::cont::CoordinateSystem("coords", p);
  }

  vtkSOADataArrayTemplate<T>* typedIn2 =
    vtkSOADataArrayTemplate<T>::FastDownCast(points->GetData());
  if (typedIn2)
  {
    auto p = DataArrayToArrayHandle<vtkSOADataArrayTemplate<T>, 3>::Wrap(typedIn2);
    return vtkm::cont::CoordinateSystem("coords", p);
  }

  vtkmDataArray<T>* typedIn3 = vtkmDataArray<T>::SafeDownCast(points->GetData());
  if (typedIn3)
  {
    return vtkm::cont::CoordinateSystem("coords", typedIn3->GetVtkmVariantArrayHandle());
  }

  typedef vtkm::Vec<T, 3> Vec3;
  Vec3* xyz = nullptr;
  return vtkm::cont::make_CoordinateSystem("coords", xyz, 0);
}
}
//------------------------------------------------------------------------------
// convert a vtkPoints array into a coordinate system
vtkm::cont::CoordinateSystem Convert(vtkPoints* points)
{
  if (points)
  {
    if (points->GetDataType() == VTK_FLOAT)
    {
      return deduce_container<vtkm::Float32>(points);
    }
    else if (points->GetDataType() == VTK_DOUBLE)
    {
      return deduce_container<vtkm::Float64>(points);
    }
  }

  // unsupported/null point set
  typedef vtkm::Vec<vtkm::Float32, 3> Vec3;
  Vec3* xyz = nullptr;
  return vtkm::cont::make_CoordinateSystem("coords", xyz, 0);
}

//------------------------------------------------------------------------------
// convert an structured grid type
vtkm::cont::DataSet Convert(vtkStructuredGrid* input, FieldsFlag fields)
{
  const int dimensionality = input->GetDataDimension();
  int dims[3];
  input->GetDimensions(dims);

  vtkm::cont::DataSet dataset;

  // first step convert the points over to an array handle
  vtkm::cont::CoordinateSystem coords = Convert(input->GetPoints());
  dataset.AddCoordinateSystem(coords);

  // second step is to create structured cellset that represe
  if (dimensionality == 1)
  {
    vtkm::cont::CellSetStructured<1> cells;
    cells.SetPointDimensions(dims[0]);
    dataset.SetCellSet(cells);
  }
  else if (dimensionality == 2)
  {
    vtkm::cont::CellSetStructured<2> cells;
    cells.SetPointDimensions(vtkm::make_Vec(dims[0], dims[1]));
    dataset.SetCellSet(cells);
  }
  else
  { // going to presume 3d for everything else
    vtkm::cont::CellSetStructured<3> cells;
    cells.SetPointDimensions(vtkm::make_Vec(dims[0], dims[1], dims[2]));
    dataset.SetCellSet(cells);
  }

  ProcessFields(input, dataset, fields);

  return dataset;
}

//------------------------------------------------------------------------------
// determine the type and call the proper Convert routine
vtkm::cont::DataSet Convert(vtkDataSet* input, FieldsFlag fields)
{
  switch (input->GetDataObjectType())
  {
    case VTK_UNSTRUCTURED_GRID:
      return Convert(vtkUnstructuredGrid::SafeDownCast(input), fields);
    case VTK_STRUCTURED_GRID:
      return Convert(vtkStructuredGrid::SafeDownCast(input), fields);
    case VTK_UNIFORM_GRID:
    case VTK_IMAGE_DATA:
      return Convert(vtkImageData::SafeDownCast(input), fields);
    case VTK_POLY_DATA:
      return Convert(vtkPolyData::SafeDownCast(input), fields);

    case VTK_UNSTRUCTURED_GRID_BASE:
    case VTK_RECTILINEAR_GRID:
    case VTK_STRUCTURED_POINTS:
    default:
      return vtkm::cont::DataSet();
  }
}

} // namespace tovtkm

namespace fromvtkm
{

namespace
{

struct ComputeExtents
{
  template <vtkm::IdComponent Dim>
  void operator()(const vtkm::cont::CellSetStructured<Dim>& cs,
    const vtkm::Id3& structuredCoordsDims, int extent[6]) const
  {
    auto extStart = cs.GetGlobalPointIndexStart();
    for (int i = 0, ii = 0; i < 3; ++i)
    {
      if (structuredCoordsDims[i] > 1)
      {
        extent[2 * i] = vtkm::VecTraits<decltype(extStart)>::GetComponent(extStart, ii++);
        extent[(2 * i) + 1] = extent[2 * i] + structuredCoordsDims[i] - 1;
      }
      else
      {
        extent[2 * i] = extent[(2 * i) + 1] = 0;
      }
    }
  }

  template <vtkm::IdComponent Dim>
  void operator()(const vtkm::cont::CellSetStructured<Dim>& cs, int extent[6]) const
  {
    auto extStart = cs.GetGlobalPointIndexStart();
    auto csDim = cs.GetPointDimensions();
    for (int i = 0; i < Dim; ++i)
    {
      extent[2 * i] = vtkm::VecTraits<decltype(extStart)>::GetComponent(extStart, i);
      extent[(2 * i) + 1] =
        extent[2 * i] + vtkm::VecTraits<decltype(csDim)>::GetComponent(csDim, i) - 1;
    }
    for (int i = Dim; i < 3; ++i)
    {
      extent[2 * i] = extent[(2 * i) + 1] = 0;
    }
  }
};
} // anonymous namespace

void PassAttributesInformation(vtkDataSetAttributes* input, vtkDataSetAttributes* output)
{
  for (int attribType = 0; attribType < vtkDataSetAttributes::NUM_ATTRIBUTES; attribType++)
  {
    vtkDataArray* attribute = input->GetAttribute(attribType);
    if (attribute == nullptr)
    {
      continue;
    }
    output->SetActiveAttribute(attribute->GetName(), attribType);
  }
}

bool Convert(const vtkm::cont::DataSet& vtkmOut, vtkRectilinearGrid* output, vtkDataSet* input)
{
  using ListCellSetStructured = vtkm::List<vtkm::cont::CellSetStructured<1>,
    vtkm::cont::CellSetStructured<2>, vtkm::cont::CellSetStructured<3> >;
  auto cellSet = vtkmOut.GetCellSet().ResetCellSetList(ListCellSetStructured{});

  using coordType =
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
      vtkm::cont::ArrayHandle<vtkm::FloatDefault>, vtkm::cont::ArrayHandle<vtkm::FloatDefault> >;
  auto coordsArray = vtkm::cont::Cast<coordType>(vtkmOut.GetCoordinateSystem().GetData());

  vtkSmartPointer<vtkDataArray> xArray =
    Convert(vtkm::cont::make_FieldPoint("xArray", coordsArray.GetStorage().GetFirstArray()));
  vtkSmartPointer<vtkDataArray> yArray =
    Convert(vtkm::cont::make_FieldPoint("yArray", coordsArray.GetStorage().GetSecondArray()));
  vtkSmartPointer<vtkDataArray> zArray =
    Convert(vtkm::cont::make_FieldPoint("zArray", coordsArray.GetStorage().GetThirdArray()));

  if (!xArray || !yArray || !zArray)
  {
    return false;
  }

  vtkm::Id3 dims(
    xArray->GetNumberOfValues(), yArray->GetNumberOfValues(), zArray->GetNumberOfValues());

  int extents[6];
  vtkm::cont::CastAndCall(cellSet, ComputeExtents{}, dims, extents);

  output->SetExtent(extents);
  output->SetXCoordinates(xArray);
  output->SetYCoordinates(yArray);
  output->SetZCoordinates(zArray);

  // Next we need to convert any extra fields from vtkm over to vtk
  if (!fromvtkm::ConvertArrays(vtkmOut, output))
  {
    return false;
  }

  // Pass information about attributes.
  PassAttributesInformation(input->GetPointData(), output->GetPointData());
  PassAttributesInformation(input->GetCellData(), output->GetCellData());

  return true;
}

bool Convert(const vtkm::cont::DataSet& vtkmOut, vtkStructuredGrid* output, vtkDataSet* input)
{
  using ListCellSetStructured = vtkm::List<vtkm::cont::CellSetStructured<1>,
    vtkm::cont::CellSetStructured<2>, vtkm::cont::CellSetStructured<3> >;
  auto cellSet = vtkmOut.GetCellSet().ResetCellSetList(ListCellSetStructured{});

  int extents[6];
  vtkm::cont::CastAndCall(cellSet, ComputeExtents{}, extents);

  vtkSmartPointer<vtkPoints> points = Convert(vtkmOut.GetCoordinateSystem());
  if (!points)
  {
    return false;
  }

  output->SetExtent(extents);
  output->SetPoints(points);

  // Next we need to convert any extra fields from vtkm over to vtk
  if (!fromvtkm::ConvertArrays(vtkmOut, output))
  {
    return false;
  }

  // Pass information about attributes.
  PassAttributesInformation(input->GetPointData(), output->GetPointData());
  PassAttributesInformation(input->GetCellData(), output->GetCellData());

  return true;
}

} // namespace fromvtkm
