/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCellType.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericCellType - define types of cells to support adaptor framework
// .SECTION Description
// see vtkCellType for explanations. Will eventually be merged with
// vtkCellType.h.

#ifndef __vtkGenericCellType_h
#define __vtkGenericCellType_h

// Higher order cells
enum
{
  VTK_HIGHER_ORDER_EDGE=60,
  VTK_HIGHER_ORDER_TRIANGLE,
  VTK_HIGHER_ORDER_QUAD,
  VTK_HIGHER_ORDER_POLYGON,
  VTK_HIGHER_ORDER_TETRAHEDRON,
  VTK_HIGHER_ORDER_WEDGE,
  VTK_HIGHER_ORDER_PYRAMID,
  VTK_HIGHER_ORDER_HEXAHEDRON
};

#endif // #ifndef __vtkGenericCellType_h
