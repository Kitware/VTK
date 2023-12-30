// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "MockDataArray.h"
#include "TestCellArrayHelpersSingleCellType1.h"

int TestCellArrayGeneric5(int, char*[])
{
  try
  {
    // clang-format off
    // Connectivity: MockDataArray<All integral types>
    TestSetDataSingleCellType<MockDataArray<vtkTypeUInt8>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<MockDataArray<vtkTypeUInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<MockDataArray<vtkTypeUInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<MockDataArray<vtkTypeUInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<MockDataArray<vtkTypeInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<MockDataArray<vtkTypeInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<MockDataArray<vtkTypeInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<MockDataArray<vtkIdType>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
