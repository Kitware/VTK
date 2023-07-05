// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
