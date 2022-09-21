/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarchingCubesPolygonCases.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkMarchingCubesPolygonCases_h
#define vtkMarchingCubesPolygonCases_h
//
// marching cubes case table for generating polygon isosurfaces
//
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSystemIncludes.h"

VTK_ABI_NAMESPACE_BEGIN
struct VTKCOMMONDATAMODEL_EXPORT vtkMarchingCubesPolygonCases
{
  int edges[17];
  static vtkMarchingCubesPolygonCases* GetCases();
};

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkMarchingCubesPolygonCases.h
