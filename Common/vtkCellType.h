/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellType.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCellType - define types of cells
// .SECTION Description
// vtkCellType defines the allowable cell types in the visualization 
// library (vtk). In vtk, datasets consist of collections of cells. 
// Different datasets consist of different cell types. The cells may be 
// explicitly represented (as in vtkPolyData), or may be implicit to the
// data type (as in vtkStructuredPoints).

#ifndef __vtkCellType_h
#define __vtkCellType_h

// To add a new cell type, define a new integer type flag here, then
// create a subclass of vtkCell to implement the proper behavior. You 
// may have to modify the following methods: vtkDataSet (and subclasses) 
// GetCell() and vtkGenericCell::SetCellType(). Also, to do the job right,
// you'll also have to modify the readers/writers and regression tests
// (example scripts) to reflect the new cell addition.

// Linear cells
#define VTK_EMPTY_CELL     0
#define VTK_VERTEX         1
#define VTK_POLY_VERTEX    2
#define VTK_LINE           3
#define VTK_POLY_LINE      4
#define VTK_TRIANGLE       5
#define VTK_TRIANGLE_STRIP 6
#define VTK_POLYGON        7
#define VTK_PIXEL          8
#define VTK_QUAD           9
#define VTK_TETRA         10
#define VTK_VOXEL         11
#define VTK_HEXAHEDRON    12
#define VTK_WEDGE         13
#define VTK_PYRAMID       14

// Quadratic, isoparametric cells
#define VTK_QUADRATIC_EDGE       21
#define VTK_QUADRATIC_TRIANGLE   22
#define VTK_QUADRATIC_QUAD       23
#define VTK_QUADRATIC_TETRA      24
#define VTK_QUADRATIC_HEXAHEDRON 25

// Special class of cells formed by convex group of points
#define VTK_CONVEX_POINT_SET 41

// Higher order cells in parametric form
#define VTK_PARAMETRIC_CURVE        51
#define VTK_PARAMETRIC_SURFACE      52
#define VTK_PARAMETRIC_TRI_SURFACE  53
#define VTK_PARAMETRIC_QUAD_SURFACE 54
#define VTK_PARAMETRIC_TETRA_REGION 55
#define VTK_PARAMETRIC_HEX_REGION   56

#endif


