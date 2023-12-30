// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpers.h"

int TestCellArrayGeneric1(int, char*[])
{
  try
  {
    // <unsigned, unsigned>
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeUInt8>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeUInt16>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeUInt32>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeUInt64>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeUInt8>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeUInt16>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeUInt32>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeUInt64>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeUInt8>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeUInt16>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeUInt32>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeUInt64>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeUInt8>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeUInt16>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeUInt32>>();
    RunTests<vtkCellArray::Generic, MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeUInt64>>();
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
