/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarchingSquaresLineCases.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkMarchingSquaresLineCases_h
#define __vtkMarchingSquaresLineCases_h
//
// Marching squares cases for generating isolines.
//
#include "vtkSystemIncludes.h"

typedef int EDGE_LIST;
struct VTK_FILTERING_EXPORT vtkMarchingSquaresLineCases
{
  EDGE_LIST edges[5];
  static vtkMarchingSquaresLineCases* GetCases();
};

#endif
