// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpers.h"

int TestCellArrayGeneric6(int, char*[])
{
  try
  {
    // clang-format off
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeInt16>, vtkAffineArray<vtkTypeUInt8>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeInt32>, vtkAffineArray<vtkTypeUInt8>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeInt64>, vtkAffineArray<vtkTypeUInt8>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeInt16>, vtkAffineArray<vtkTypeUInt16>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeInt32>, vtkAffineArray<vtkTypeUInt16>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeInt64>, vtkAffineArray<vtkTypeUInt16>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeInt16>, vtkAffineArray<vtkTypeUInt32>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeInt32>, vtkAffineArray<vtkTypeUInt32>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeInt64>, vtkAffineArray<vtkTypeUInt32>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeInt16>, vtkAffineArray<vtkTypeUInt64>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeInt32>, vtkAffineArray<vtkTypeUInt64>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeInt64>, vtkAffineArray<vtkTypeUInt64>>();
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
