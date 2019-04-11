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
#include "PolyDataConverter.h"
#include "ImageDataConverter.h"
#include "Storage.h"
#include "UnstructuredGridConverter.h"

#include "vtkmCellSetSingleType.h"
#include "vtkmDataArray.h"

// datasets we support
#include "vtkCellArray.h"
#include "vtkCellTypes.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.hxx>
#include <vtkm/cont/Field.h>

namespace tovtkm {

namespace {

template <typename T>
vtkm::cont::CoordinateSystem deduce_container(vtkPoints *points)
{
  typedef vtkm::Vec<T, 3> Vec3;

  vtkAOSDataArrayTemplate<T> *typedIn =
      vtkAOSDataArrayTemplate<T>::FastDownCast(points->GetData());
  if (typedIn)
  {
    typedef tovtkm::vtkAOSArrayContainerTag TagType;
    typedef vtkm::cont::internal::Storage<Vec3, TagType> StorageType;
    StorageType storage(typedIn);
    vtkm::cont::ArrayHandle<Vec3, TagType> p(storage);
    return vtkm::cont::CoordinateSystem("coords", p);
  }

  vtkSOADataArrayTemplate<T> *typedIn2 =
      vtkSOADataArrayTemplate<T>::FastDownCast(points->GetData());
  if (typedIn2)
  {
    typedef tovtkm::vtkSOAArrayContainerTag TagType;
    typedef vtkm::cont::internal::Storage<Vec3, TagType> StorageType;
    StorageType storage(typedIn2);
    vtkm::cont::ArrayHandle<Vec3, TagType> p(storage);
    return vtkm::cont::CoordinateSystem("coords", p);
  }

  vtkmDataArray<T> *typedIn3 =
      vtkmDataArray<T>::SafeDownCast(points->GetData());
  if (typedIn3)
  {
    return vtkm::cont::CoordinateSystem("coords", typedIn3->GetVtkmVariantArrayHandle());
  }

  typedef vtkm::Vec<T, 3> Vec3;
  Vec3 *xyz = nullptr;
  return vtkm::cont::make_CoordinateSystem("coords", xyz, 0);
}
}
//------------------------------------------------------------------------------
// convert a vtkPoints array into a coordinate system
vtkm::cont::CoordinateSystem Convert(vtkPoints *points)
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
  Vec3 *xyz = nullptr;
  return vtkm::cont::make_CoordinateSystem("coords", xyz, 0);
}

//------------------------------------------------------------------------------
// convert an structured grid type
vtkm::cont::DataSet Convert(vtkStructuredGrid *input, FieldsFlag fields)
{
  const int dimensionality = input->GetDataDimension();
  int dims[3]; input->GetDimensions(dims);

  vtkm::cont::DataSet dataset;

  // first step convert the points over to an array handle
  vtkm::cont::CoordinateSystem coords = Convert(input->GetPoints());
  dataset.AddCoordinateSystem(coords);

  // second step is to create structured cellset that represe
  if(dimensionality == 1)
  {
    vtkm::cont::CellSetStructured<1> cells("cells");
    cells.SetPointDimensions(dims[0]);
    dataset.AddCellSet(cells);
  }
  else if(dimensionality == 2)
  {
    vtkm::cont::CellSetStructured<2> cells("cells");
    cells.SetPointDimensions(vtkm::make_Vec(dims[0],dims[1]));
    dataset.AddCellSet(cells);
  }
  else
  { //going to presume 3d for everything else
    vtkm::cont::CellSetStructured<3> cells("cells");
    cells.SetPointDimensions(vtkm::make_Vec(dims[0],dims[1],dims[2]));
    dataset.AddCellSet(cells);
  }

  ProcessFields(input, dataset, fields);

  return dataset;
}

//------------------------------------------------------------------------------
// determine the type and call the proper Convert routine
vtkm::cont::DataSet Convert(vtkDataSet *input, FieldsFlag fields)
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
