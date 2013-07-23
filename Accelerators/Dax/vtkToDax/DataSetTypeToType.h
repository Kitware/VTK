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

#ifndef vtkToDax_DataSetTypeToType_h
#define vtkToDax_DataSetTypeToType_h

#include "vtkType.h"

#include <dax/cont/UniformGrid.h>
#include <dax/cont/UnstructuredGrid.h>

class vtkImageData;
class vtkUniformGrid;
class vtkStructuredGrid;
class vtkUnstructuredGrid;

//header that determines the correct cell and datatypes that can be used together
#include "CellTypeAndDataType.h"
#include "Containers.h"

namespace vtkToDax
{

//empty implementation for dataset we don't support
template<typename CellTypeToTypeDef, typename vtkDataSetType> struct DataSetTypeToType
{
  typedef CellTypeToTypeDef CellTypeToType;
  typedef typename CellTypeToTypeDef::DaxCellType DaxCellType;
  enum {VTKDataSetType=-1};
  enum {Valid=false};
  typedef bool DaxDataSetType;
};

template<typename CellTypeToTypeDef> struct DataSetTypeToType<CellTypeToTypeDef,vtkImageData>
{
  typedef CellTypeToTypeDef CellTypeToType;
  typedef typename CellTypeToTypeDef::DaxCellType DaxCellType;
  enum {VTKDataSetType=VTK_IMAGE_DATA};
  enum {Valid=(CellTypeAndDataType<VTK_IMAGE_DATA,CellTypeToTypeDef::VTKCellType>::Valid)};
  typedef dax::cont::UniformGrid<> DaxDataSetType;
};

template<typename CellTypeToTypeDef> struct DataSetTypeToType<CellTypeToTypeDef,vtkUniformGrid>
{
  typedef CellTypeToTypeDef CellTypeToType;
  typedef typename CellTypeToTypeDef::DaxCellType DaxCellType;
  enum {VTKDataSetType=VTK_UNIFORM_GRID};
  enum {Valid=(CellTypeAndDataType<VTK_UNIFORM_GRID,CellTypeToTypeDef::VTKCellType>::Valid)};
  typedef dax::cont::UniformGrid<> DaxDataSetType;
};

template<typename CellTypeToTypeDef> struct DataSetTypeToType<CellTypeToTypeDef,vtkUnstructuredGrid>
{
  typedef CellTypeToTypeDef CellTypeToType;
  typedef typename CellTypeToTypeDef::DaxCellType DaxCellType;
  enum {VTKDataSetType=VTK_UNSTRUCTURED_GRID};
  enum {Valid=(CellTypeAndDataType<VTK_UNSTRUCTURED_GRID,CellTypeToTypeDef::VTKCellType>::Valid)};
  typedef dax::cont::UnstructuredGrid<DaxCellType,
          vtkToDax::vtkTopologyContainerTag<CellTypeToType>,
          vtkToDax::vtkPointsContainerTag>
          DaxDataSetType;
};
}

#endif // vtkToDax_DataSetTypeToType_h
