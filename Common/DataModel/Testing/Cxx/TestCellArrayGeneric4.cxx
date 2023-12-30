// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpers.h"

int TestCellArrayGeneric4(int, char*[])
{
  try
  {
    // <signed, unsigned>
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeInt16>, MockDataArray<vtkTypeUInt8>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeInt16>, MockDataArray<vtkTypeUInt16>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeInt16>, MockDataArray<vtkTypeUInt32>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeInt16>, MockDataArray<vtkTypeUInt64>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeInt32>, MockDataArray<vtkTypeUInt8>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeInt32>, MockDataArray<vtkTypeUInt16>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeInt32>, MockDataArray<vtkTypeUInt32>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeInt32>, MockDataArray<vtkTypeUInt64>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeInt64>, MockDataArray<vtkTypeUInt8>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeInt64>, MockDataArray<vtkTypeUInt16>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeInt64>, MockDataArray<vtkTypeUInt32>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeInt64>, MockDataArray<vtkTypeUInt64>>();
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
