/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CellType.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkCellType - define types of cells
// .SECTION Description
// vtkCellType defines the allowable cell types in the visualization 
// library (vtk). In vtk, datasets consist of collections of cells. 
// Different datasets consist of different cell types. The cells may be 
// explicitly represented (as in vtkPolyData), or may be implicit to the
// data type (vtkStructuredPoints).

#ifndef __vtkCellTypes_h
#define __vtkCellTypes_h

#define vtkNULL_ELEMENT 0
#define vtkVERTEX 1
#define vtkPOLY_VERTEX 2
#define vtkLINE 3
#define vtkPOLY_LINE 4
#define vtkTRIANGLE 5
#define vtkTRIANGLE_STRIP 6
#define vtkPOLYGON 7
#define vtkPIXEL 8
#define vtkQUAD 9
#define vtkTETRA 10
#define vtkVOXEL 11
#define vtkHEXAHEDRON 12

#endif


