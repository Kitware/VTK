// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpers.h"

int TestCellArrayGeneric7(int, char*[])
{
  try
  {
    // clang-format off
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeUInt8>, vtkAffineArray<vtkTypeInt16>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeUInt16>, vtkAffineArray<vtkTypeInt16>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeUInt32>, vtkAffineArray<vtkTypeInt16>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeUInt64>, vtkAffineArray<vtkTypeInt16>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeUInt8>, vtkAffineArray<vtkTypeInt32>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeUInt16>, vtkAffineArray<vtkTypeInt32>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeUInt32>, vtkAffineArray<vtkTypeInt32>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeUInt64>, vtkAffineArray<vtkTypeInt32>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeUInt8>, vtkAffineArray<vtkTypeInt64>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeUInt16>, vtkAffineArray<vtkTypeInt64>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeUInt32>, vtkAffineArray<vtkTypeInt64>>();
    ::RunTests<vtkCellArray::Generic, true, MockDataArray<vtkTypeUInt64>, vtkAffineArray<vtkTypeInt64>>();
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
