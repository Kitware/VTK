// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpers.h"
#include "vtkIdTypeArray.h"

int TestCellArrayGeneric17(int, char*[])
{
  try
  {
    // clang-format off
    // Offsets: All integral types, Connectivity: vtkIdTypeArray
    ::RunTests<vtkCellArray::Generic, true, vtkIdTypeArray, vtkAffineArray<vtkTypeUInt8>>();
    ::RunTests<vtkCellArray::Generic, true, vtkIdTypeArray, vtkAffineArray<vtkTypeUInt16>>();
    ::RunTests<vtkCellArray::Generic, true, vtkIdTypeArray, vtkAffineArray<vtkTypeUInt32>>();
    ::RunTests<vtkCellArray::Generic, true, vtkIdTypeArray, vtkAffineArray<vtkTypeUInt64>>();
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
