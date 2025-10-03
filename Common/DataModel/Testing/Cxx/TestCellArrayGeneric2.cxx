// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpers.h"

int TestCellArrayGeneric2(int, char*[])
{
  try
  {
    // <unsigned, signed>
    // clang-format off
    ::RunTests<vtkCellArray::Generic, false, MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeInt16>>();
    ::RunTests<vtkCellArray::Generic, false, MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeInt32>>();
    ::RunTests<vtkCellArray::Generic, false, MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeInt64>>();
    ::RunTests<vtkCellArray::Generic, false, MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeInt16>>();
    ::RunTests<vtkCellArray::Generic, false, MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeInt32>>();
    ::RunTests<vtkCellArray::Generic, false, MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeInt64>>();
    ::RunTests<vtkCellArray::Generic, false, MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeInt16>>();
    ::RunTests<vtkCellArray::Generic, false, MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeInt32>>();
    ::RunTests<vtkCellArray::Generic, false, MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeInt64>>();
    ::RunTests<vtkCellArray::Generic, false, MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeInt16>>();
    ::RunTests<vtkCellArray::Generic, false, MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeInt32>>();
    ::RunTests<vtkCellArray::Generic, false, MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeInt64>>();
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
