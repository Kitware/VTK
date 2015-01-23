/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellType.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#ifndef vtkCellType_h
#define vtkCellType_h

// To add a new cell type, define a new integer type flag here, then
// create a subclass of vtkCell to implement the proper behavior. You
// may have to modify the following methods: vtkDataSet (and subclasses)
// GetCell() and vtkGenericCell::SetCellType(). Also, to do the job right,
// you'll also have to modify some filters (vtkGeometryFilter...) and
// regression tests (example scripts) to reflect the new cell addition.
// Also, make sure to update vtkCellTypesStrings in vtkCellTypes.cxx
// and the vtkCellTypes::IsLinear method in vtkCellTypes.h.

// .SECTION Caveats
// An unstructured grid stores the types of its cells as a
// unsigned char array. Therefore, the maximum encoding number for a cell type
// is 255.

typedef enum {
  // Linear cells
  VTK_EMPTY_CELL       = 0,
  VTK_VERTEX           = 1,
  VTK_POLY_VERTEX      = 2,
  VTK_LINE             = 3,
  VTK_POLY_LINE        = 4,
  VTK_TRIANGLE         = 5,
  VTK_TRIANGLE_STRIP   = 6,
  VTK_POLYGON          = 7,
  VTK_PIXEL            = 8,
  VTK_QUAD             = 9,
  VTK_TETRA            = 10,
  VTK_VOXEL            = 11,
  VTK_HEXAHEDRON       = 12,
  VTK_WEDGE            = 13,
  VTK_PYRAMID          = 14,
  VTK_PENTAGONAL_PRISM = 15,
  VTK_HEXAGONAL_PRISM  = 16,

  // Quadratic, isoparametric cells
  VTK_QUADRATIC_EDGE                   = 21,
  VTK_QUADRATIC_TRIANGLE               = 22,
  VTK_QUADRATIC_QUAD                   = 23,
  VTK_QUADRATIC_POLYGON                = 36,
  VTK_QUADRATIC_TETRA                  = 24,
  VTK_QUADRATIC_HEXAHEDRON             = 25,
  VTK_QUADRATIC_WEDGE                  = 26,
  VTK_QUADRATIC_PYRAMID                = 27,
  VTK_BIQUADRATIC_QUAD                 = 28,
  VTK_TRIQUADRATIC_HEXAHEDRON          = 29,
  VTK_QUADRATIC_LINEAR_QUAD            = 30,
  VTK_QUADRATIC_LINEAR_WEDGE           = 31,
  VTK_BIQUADRATIC_QUADRATIC_WEDGE      = 32,
  VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON = 33,
  VTK_BIQUADRATIC_TRIANGLE             = 34,

  // Cubic, isoparametric cell
  VTK_CUBIC_LINE                       = 35,

  // Special class of cells formed by convex group of points
  VTK_CONVEX_POINT_SET = 41,

  // Polyhedron cell (consisting of polygonal faces)
  VTK_POLYHEDRON = 42,

  // Higher order cells in parametric form
  VTK_PARAMETRIC_CURVE        = 51,
  VTK_PARAMETRIC_SURFACE      = 52,
  VTK_PARAMETRIC_TRI_SURFACE  = 53,
  VTK_PARAMETRIC_QUAD_SURFACE = 54,
  VTK_PARAMETRIC_TETRA_REGION = 55,
  VTK_PARAMETRIC_HEX_REGION   = 56,

  // Higher order cells
  VTK_HIGHER_ORDER_EDGE        = 60,
  VTK_HIGHER_ORDER_TRIANGLE    = 61,
  VTK_HIGHER_ORDER_QUAD        = 62,
  VTK_HIGHER_ORDER_POLYGON     = 63,
  VTK_HIGHER_ORDER_TETRAHEDRON = 64,
  VTK_HIGHER_ORDER_WEDGE       = 65,
  VTK_HIGHER_ORDER_PYRAMID     = 66,
  VTK_HIGHER_ORDER_HEXAHEDRON  = 67,

  VTK_NUMBER_OF_CELL_TYPES
} VTKCellType;

#endif
// VTK-HeaderTest-Exclude: vtkCellType.h
