// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMarchingCubesTriangleCases_h
#define vtkMarchingCubesTriangleCases_h
//
// marching cubes case table for generating isosurfaces
//
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSystemIncludes.h"

VTK_ABI_NAMESPACE_BEGIN
struct VTKCOMMONDATAMODEL_EXPORT vtkMarchingCubesTriangleCases
{
  int edges[16];
  static vtkMarchingCubesTriangleCases* GetCases();
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkMarchingCubesTriangleCases.h
