// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpers.h"
#include "vtkIdTypeArray.h"

int TestCellArrayGeneric18(int, char*[])
{
  try
  {
    // clang-format off
    ::RunTests<vtkCellArray::Generic, true, vtkIdTypeArray, vtkAffineArray<vtkTypeInt16>>();
    ::RunTests<vtkCellArray::Generic, true, vtkIdTypeArray, vtkAffineArray<vtkTypeInt32>>();
    ::RunTests<vtkCellArray::Generic, true, vtkIdTypeArray, vtkAffineArray<vtkTypeInt64>>();
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
