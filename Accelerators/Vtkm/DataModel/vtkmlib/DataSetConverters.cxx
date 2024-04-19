// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

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
#include <vtkm/cont/Field.h>

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

namespace
{

template <typename T>
vtkm::cont::CoordinateSystem deduce_container(vtkPoints* points)
{
  typedef vtkm::Vec<T, 3> Vec3;

  vtkAOSDataArrayTemplate<T>* typedIn = vtkAOSDataArrayTemplate<T>::FastDownCast(points->GetData());
  if (typedIn)
  {
    auto p = vtkDataArrayToArrayHandle(typedIn);
    return vtkm::cont::CoordinateSystem("coords", p);
  }

  vtkSOADataArrayTemplate<T>* typedIn2 =
    vtkSOADataArrayTemplate<T>::FastDownCast(points->GetData());
  if (typedIn2)
  {
    auto p = vtkDataArrayToArrayHandle(typedIn2);
    return vtkm::cont::CoordinateSystem("coords", p);
  }

  vtkmDataArray<T>* typedIn3 = vtkmDataArray<T>::SafeDownCast(points->GetData());
  if (typedIn3)
  {
    return vtkm::cont::CoordinateSystem("coords", typedIn3->GetVtkmUnknownArrayHandle());
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
// convert rectilinear coordinates
template <typename T>
vtkm::cont::CoordinateSystem ConvertRectilinearPoints(
  vtkDataArray* xArray, vtkDataArray* yArray, vtkDataArray* zArray)
{
  vtkDataArray* vtkCompArrays[3] = { xArray, yArray, zArray };
  vtkm::cont::ArrayHandle<T> vtkmCompArrays[3];

  for (int i = 0; i < 3; ++i)
  {
    vtkAOSDataArrayTemplate<T>* aos = vtkAOSDataArrayTemplate<T>::FastDownCast(vtkCompArrays[i]);
    if (aos)
    {
      vtkmCompArrays[i] = vtkAOSDataArrayToFlatArrayHandle(aos);
      continue;
    }

    vtkSOADataArrayTemplate<T>* soa = vtkSOADataArrayTemplate<T>::FastDownCast(vtkCompArrays[i]);
    if (soa)
    {
      vtkmCompArrays[i] = vtkSOADataArrayToComponentArrayHandle(soa, 0);
      continue;
    }

    throw vtkm::cont::ErrorBadType("Unexpected rectilinear component array type (VTK)");
  }

  return vtkm::cont::CoordinateSystem("coords",
    vtkm::cont::make_ArrayHandleCartesianProduct(
      vtkmCompArrays[0], vtkmCompArrays[1], vtkmCompArrays[2]));
}

//------------------------------------------------------------------------------
// convert a rectilinear grid type
vtkm::cont::DataSet Convert(vtkRectilinearGrid* input, FieldsFlag fields)
{
  const int dimensionality = input->GetDataDimension();
  int dims[3];
  input->GetDimensions(dims);

  int extent[6];
  input->GetExtent(extent);

  vtkm::cont::DataSet dataset;

  // first step, convert the points x, y aqnd z arrays over
  if (input->GetXCoordinates()->GetDataType() == VTK_DOUBLE)
  {
    dataset.AddCoordinateSystem(ConvertRectilinearPoints<double>(
      input->GetXCoordinates(), input->GetYCoordinates(), input->GetZCoordinates()));
  }
  else // assume float
  {
    dataset.AddCoordinateSystem(ConvertRectilinearPoints<float>(
      input->GetXCoordinates(), input->GetYCoordinates(), input->GetZCoordinates()));
  }

  // second step is to create structured cellset that represe
  if (dimensionality == 1)
  {
    vtkm::cont::CellSetStructured<1> cells;
    if (dims[0] > 1)
    {
      cells.SetPointDimensions(dims[0]);
      cells.SetGlobalPointIndexStart(extent[0]);
    }
    else if (dims[1] > 1)
    {
      cells.SetPointDimensions(dims[1]);
      cells.SetGlobalPointIndexStart(extent[2]);
    }
    else
    {
      cells.SetPointDimensions(dims[2]);
      cells.SetGlobalPointIndexStart(extent[4]);
    }
    dataset.SetCellSet(cells);
  }
  else if (dimensionality == 2)
  {
    vtkm::cont::CellSetStructured<2> cells;
    if (dims[0] == 1)
    {
      cells.SetPointDimensions(vtkm::make_Vec(dims[1], dims[2]));
      cells.SetGlobalPointIndexStart(vtkm::make_Vec(extent[2], extent[4]));
    }
    else if (dims[1] == 1)
    {
      cells.SetPointDimensions(vtkm::make_Vec(dims[0], dims[2]));
      cells.SetGlobalPointIndexStart(vtkm::make_Vec(extent[0], extent[4]));
    }
    else
    {
      cells.SetPointDimensions(vtkm::make_Vec(dims[0], dims[1]));
      cells.SetGlobalPointIndexStart(vtkm::make_Vec(extent[0], extent[2]));
    }

    dataset.SetCellSet(cells);
  }
  else // going to presume 3d for everything else
  {
    vtkm::cont::CellSetStructured<3> cells;
    cells.SetPointDimensions(vtkm::make_Vec(dims[0], dims[1], dims[2]));
    cells.SetGlobalPointIndexStart(vtkm::make_Vec(extent[0], extent[2], extent[4]));
    dataset.SetCellSet(cells);
  }

  ProcessFields(input, dataset, fields);

  return dataset;
}

//------------------------------------------------------------------------------
// determine the type and call the proper Convert routine
vtkm::cont::DataSet Convert(vtkDataSet* input, FieldsFlag fields)
{
  auto typeId = input->GetDataObjectType();
  switch (typeId)
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
    case VTK_RECTILINEAR_GRID:
      return Convert(vtkRectilinearGrid::SafeDownCast(input), fields);

    case VTK_UNSTRUCTURED_GRID_BASE:
    case VTK_STRUCTURED_POINTS:
    default:
      const std::string typeStr = vtkDataObjectTypes::GetClassNameFromTypeId(typeId);
      const std::string errMsg = "Unable to convert " + typeStr + " to vtkm::cont::DataSet";
      throw vtkm::cont::ErrorBadType(errMsg);
  }
}

VTK_ABI_NAMESPACE_END
} // namespace tovtkm

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN

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
    vtkm::cont::CellSetStructured<2>, vtkm::cont::CellSetStructured<3>>;
  auto cellSet = vtkmOut.GetCellSet().ResetCellSetList(ListCellSetStructured{});

  vtkSmartPointer<vtkDataArray> xArray, yArray, zArray;
  if (vtkmOut.GetCoordinateSystem().GetData().template IsValueType<vtkm::Float32>())
  {
    using coordArrayType =
      vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::Float32>,
        vtkm::cont::ArrayHandle<vtkm::Float32>, vtkm::cont::ArrayHandle<vtkm::Float32>>;
    coordArrayType coordsArray;
    vtkmOut.GetCoordinateSystem().GetData().AsArrayHandle(coordsArray);

    xArray.TakeReference(
      Convert(vtkm::cont::make_FieldPoint("xArray", coordsArray.GetFirstArray())));
    yArray.TakeReference(
      Convert(vtkm::cont::make_FieldPoint("yArray", coordsArray.GetSecondArray())));
    zArray.TakeReference(
      Convert(vtkm::cont::make_FieldPoint("zArray", coordsArray.GetThirdArray())));
  }
  else // vtkm::Float64
  {
    using coordArrayType =
      vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::Float64>,
        vtkm::cont::ArrayHandle<vtkm::Float64>, vtkm::cont::ArrayHandle<vtkm::Float64>>;
    coordArrayType coordsArray;
    vtkmOut.GetCoordinateSystem().GetData().AsArrayHandle(coordsArray);

    xArray.TakeReference(
      Convert(vtkm::cont::make_FieldPoint("xArray", coordsArray.GetFirstArray())));
    yArray.TakeReference(
      Convert(vtkm::cont::make_FieldPoint("yArray", coordsArray.GetSecondArray())));
    zArray.TakeReference(
      Convert(vtkm::cont::make_FieldPoint("zArray", coordsArray.GetThirdArray())));
  }

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
    vtkm::cont::CellSetStructured<2>, vtkm::cont::CellSetStructured<3>>;
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

VTK_ABI_NAMESPACE_END
} // namespace fromvtkm
