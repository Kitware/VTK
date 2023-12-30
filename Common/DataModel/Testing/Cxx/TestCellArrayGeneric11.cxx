// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "MockDataArray.h"
#include "TestCellArrayHelpersSingleCellType2.h"

int TestCellArrayGeneric11(int, char*[])
{
  try
  {
    // clang-format off
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt16, MockDataArray<vtkTypeInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt16, MockDataArray<vtkTypeInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt16, MockDataArray<vtkTypeInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt32, MockDataArray<vtkTypeInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt32, MockDataArray<vtkTypeInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt32, MockDataArray<vtkTypeInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt64, MockDataArray<vtkTypeInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt64, MockDataArray<vtkTypeInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt64, MockDataArray<vtkTypeInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
