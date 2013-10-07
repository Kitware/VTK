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

#ifndef daxToVtk_CellTypeToType_h
#define daxToVtk_CellTypeToType_h
#include <dax/CellTraits.h>

namespace daxToVtk
{
template<typename T> struct CellTypeToType;
template<> struct CellTypeToType<dax::CellTagLine>
{
  enum {VTKCellType=VTK_LINE};
  enum {NUM_POINTS=dax::CellTraits<dax::CellTagLine>::NUM_VERTICES};
  typedef vtkLine VTKCellClass;
};

template<> struct CellTypeToType<dax::CellTagHexahedron>
{
  enum {VTKCellType=VTK_HEXAHEDRON};
  enum {NUM_POINTS=dax::CellTraits<dax::CellTagHexahedron>::NUM_VERTICES};
  typedef vtkHexahedron VTKCellClass;
};

template<> struct CellTypeToType<dax::CellTagQuadrilateral>
{
  enum {VTKCellType=VTK_QUAD};
  enum {NUM_POINTS=dax::CellTraits<dax::CellTagQuadrilateral>::NUM_VERTICES};
  typedef vtkQuad VTKCellClass;
};

template<> struct CellTypeToType<dax::CellTagTetrahedron>
{
  enum {VTKCellType=VTK_TETRA};
  enum {NUM_POINTS=dax::CellTraits<dax::CellTagTetrahedron>::NUM_VERTICES};
  typedef vtkTetra VTKCellClass;
};

template<> struct CellTypeToType<dax::CellTagTriangle>
{
  enum {VTKCellType=VTK_TRIANGLE};
  enum {NUM_POINTS=dax::CellTraits<dax::CellTagTriangle>::NUM_VERTICES};
  typedef vtkTriangle VTKCellClass;
};

template<> struct CellTypeToType<dax::CellTagVoxel>
{
  enum {VTKCellType=VTK_VOXEL};
  enum {NUM_POINTS=dax::CellTraits<dax::CellTagVoxel>::NUM_VERTICES};
  typedef vtkVoxel VTKCellClass;
};

template<> struct CellTypeToType<dax::CellTagVertex>
{
  enum {VTKCellType=VTK_VERTEX};
  enum {NUM_POINTS=dax::CellTraits<dax::CellTagVertex>::NUM_VERTICES};
  typedef vtkVertex VTKCellClass;
};

template<> struct CellTypeToType<dax::CellTagWedge>
{
  enum {VTKCellType=VTK_WEDGE};
  enum {NUM_POINTS=dax::CellTraits<dax::CellTagWedge>::NUM_VERTICES};
  typedef vtkWedge VTKCellClass;
};

}
#endif
