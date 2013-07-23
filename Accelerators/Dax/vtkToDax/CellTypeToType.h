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

#ifndef vtkToDax_CellTypeToType_h
#define vtkToDax_CellTypeToType_h

#include "vtkCellType.h"


class vtkLine;
class vtkHexahedron;
class vtkQuad;
class vtkTetra;
class vtkTriangle;
class vtkVoxel;
class vtkWedge;

#include <dax/CellTag.h>

//ToDo the output type needs to be moved to a separate header that
//is per algorithm output type, that maps the input cell type to the output
//cell type.
namespace vtkToDax
{
template<typename T> struct CellTypeToType;
template<> struct CellTypeToType<vtkLine>
{
  enum {VTKCellType=VTK_LINE};
  enum {NUM_POINTS=2};
  typedef dax::CellTagLine DaxCellType;
};

template<> struct CellTypeToType<vtkHexahedron>
{
  enum {VTKCellType=VTK_HEXAHEDRON};
  enum {NUM_POINTS=8};
  typedef dax::CellTagHexahedron DaxCellType;
};

template<> struct CellTypeToType<vtkQuad>
{
  enum {VTKCellType=VTK_QUAD};
  enum {NUM_POINTS=4};
  typedef dax::CellTagQuadrilateral DaxCellType;
};


template<> struct CellTypeToType<vtkTetra>
{
  enum {VTKCellType=VTK_TETRA};
  enum {NUM_POINTS=4};
  typedef dax::CellTagTetrahedron DaxCellType;
};

template<> struct CellTypeToType<vtkTriangle>
{
  enum {VTKCellType=VTK_TRIANGLE};
  enum {NUM_POINTS=3};
  typedef dax::CellTagTriangle DaxCellType;
};

template<> struct CellTypeToType<vtkVoxel>
{
  enum {VTKCellType=VTK_VOXEL};
  enum {NUM_POINTS=8};
  typedef dax::CellTagVoxel DaxCellType;
};

template<> struct CellTypeToType<vtkVertex>
{
  enum {VTKCellType=VTK_VERTEX};
  enum {NUM_POINTS=1};
  typedef dax::CellTagVertex DaxCellType;
};

template<> struct CellTypeToType<vtkWedge>
{
  enum {VTKCellType=VTK_WEDGE};
  enum {NUM_POINTS=6};
  typedef dax::CellTagWedge DaxCellType;
};
}


#endif // vtkToDax_CellTypeToType_h
