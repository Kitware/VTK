// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMarchingSquaresLineCases.h"

// Note: the following code is placed here to deal with cross-library
// symbol export and import on Microsoft compilers.
VTK_ABI_NAMESPACE_BEGIN
static vtkMarchingSquaresLineCases VTK_MARCHING_SQUARES_LINECASES[] = {
  { { -1, -1, -1, -1, -1 } },
  { { 0, 3, -1, -1, -1 } },
  { { 1, 0, -1, -1, -1 } },
  { { 1, 3, -1, -1, -1 } },
  { { 2, 1, -1, -1, -1 } },
  { { 0, 3, 2, 1, -1 } },
  { { 2, 0, -1, -1, -1 } },
  { { 2, 3, -1, -1, -1 } },
  { { 3, 2, -1, -1, -1 } },
  { { 0, 2, -1, -1, -1 } },
  { { 1, 0, 3, 2, -1 } },
  { { 1, 2, -1, -1, -1 } },
  { { 3, 1, -1, -1, -1 } },
  { { 0, 1, -1, -1, -1 } },
  { { 3, 0, -1, -1, -1 } },
  { { -1, -1, -1, -1, -1 } },
};

vtkMarchingSquaresLineCases* vtkMarchingSquaresLineCases::GetCases()
{
  return VTK_MARCHING_SQUARES_LINECASES;
}
VTK_ABI_NAMESPACE_END
