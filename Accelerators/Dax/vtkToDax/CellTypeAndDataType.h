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

#ifndef vtkToDax_CellTypeAndDataType_h
#define vtkToDax_CellTypeAndDataType_h

#include "vtkType.h"
#include "vtkCellType.h"


namespace vtkToDax
{
//By default we list any combination as being invalid
template<int DataSetType, int CellType>
struct CellTypeAndDataType
{
  enum{Valid=0};
};

//we than specialize all the valid combinations of dataset and cell types
//that Dax currently supports
template<> struct CellTypeAndDataType<VTK_IMAGE_DATA,VTK_VOXEL>{enum{Valid=1};};
template<> struct CellTypeAndDataType<VTK_UNIFORM_GRID,VTK_VOXEL>{enum{Valid=1};};

template<> struct CellTypeAndDataType<VTK_UNSTRUCTURED_GRID,VTK_LINE>{enum{Valid=1};};
template<> struct CellTypeAndDataType<VTK_UNSTRUCTURED_GRID,VTK_HEXAHEDRON>{enum{Valid=1};};
template<> struct CellTypeAndDataType<VTK_UNSTRUCTURED_GRID,VTK_QUAD>{enum{Valid=1};};
template<> struct CellTypeAndDataType<VTK_UNSTRUCTURED_GRID,VTK_TETRA>{enum{Valid=1};};
template<> struct CellTypeAndDataType<VTK_UNSTRUCTURED_GRID,VTK_TRIANGLE>{enum{Valid=1};};
template<> struct CellTypeAndDataType<VTK_UNSTRUCTURED_GRID,VTK_WEDGE>{enum{Valid=1};};

template<> struct CellTypeAndDataType<VTK_STRUCTURED_GRID,VTK_HEXAHEDRON>{enum{Valid=1};};
}


#endif // vtkToDax_CellTypeAndDataType_h
