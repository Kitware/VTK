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
#include "Storage.h"
#include "UnstructuredGridConverter.h"

#include "vtkmCellSetSingleType.h"

// datasets we support
#include "vtkCellArray.h"
#include "vtkCellTypes.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
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

  typedef vtkm::Vec<T, 3> Vec3;
  Vec3 *xyz = NULL;
  return vtkm::cont::CoordinateSystem("coords", xyz, 0);
}
}
//------------------------------------------------------------------------------
// convert a vtkPoints array into a coordinate system
vtkm::cont::CoordinateSystem Convert(vtkPoints *points)
{
  if (points->GetDataType() == VTK_FLOAT)
  {
    return deduce_container<vtkm::Float32>(points);
  }
  else if (points->GetDataType() == VTK_DOUBLE)
  {
    return deduce_container<vtkm::Float64>(points);
  }
  else
  {
    // unsupported point set
    typedef vtkm::Vec<vtkm::Float32, 3> Vec3;
    Vec3 *xyz = NULL;
    return vtkm::cont::CoordinateSystem("coords", xyz, 0);
  }
}

//------------------------------------------------------------------------------
// convert an image data type
vtkm::cont::DataSet Convert(vtkImageData *input)
{
  int extent[6];
  input->GetExtent(extent);
  double vorigin[3];
  input->GetOrigin(vorigin);
  double vspacing[3];
  input->GetSpacing(vspacing);
  int vdims[3];
  input->GetDimensions(vdims);

  vtkm::Vec<vtkm::FloatDefault, 3> origin(extent[0]*vspacing[0] + vorigin[0],
                                          extent[2]*vspacing[1] + vorigin[1],
                                          extent[4]*vspacing[2] + vorigin[2]);
  vtkm::Vec<vtkm::FloatDefault, 3> spacing(vspacing[0],
                                           vspacing[1],
                                           vspacing[2]);
  vtkm::Id3 dims(vdims[0], vdims[1], vdims[2]);

  vtkm::cont::DataSet dataset =
      vtkm::cont::DataSetBuilderUniform::Create(dims, origin, spacing);

  return dataset;
}

//------------------------------------------------------------------------------
// convert an structured grid type
vtkm::cont::DataSet Convert(vtkStructuredGrid *input)
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
  return dataset;
}

//------------------------------------------------------------------------------
// determine the type and call the proper Convert routine
vtkm::cont::DataSet Convert(vtkDataSet *input)
{
  switch (input->GetDataObjectType())
  {
  case VTK_UNSTRUCTURED_GRID:
    return Convert(vtkUnstructuredGrid::SafeDownCast(input));
  case VTK_STRUCTURED_GRID:
    return Convert(vtkStructuredGrid::SafeDownCast(input));
  case VTK_UNIFORM_GRID:
  case VTK_IMAGE_DATA:
    return Convert(vtkImageData::SafeDownCast(input));
  case VTK_POLY_DATA:
    return Convert(vtkPolyData::SafeDownCast(input));

  case VTK_UNSTRUCTURED_GRID_BASE:
  case VTK_RECTILINEAR_GRID:
  case VTK_STRUCTURED_POINTS:
  default:
    return vtkm::cont::DataSet();
  }
}

} // namespace tovtkm
