// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpersSingleCellType2.h"
#include "vtkAOSDataArrayTemplate.h"

int TestCellArrayGeneric15(int, char*[])
{
  try
  {
    // clang-format off
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt16, vtkAOSDataArrayTemplate<vtkTypeInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt16, vtkAOSDataArrayTemplate<vtkTypeInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt16, vtkAOSDataArrayTemplate<vtkTypeInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt32, vtkAOSDataArrayTemplate<vtkTypeInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt32, vtkAOSDataArrayTemplate<vtkTypeInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt32, vtkAOSDataArrayTemplate<vtkTypeInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt64, vtkAOSDataArrayTemplate<vtkTypeInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt64, vtkAOSDataArrayTemplate<vtkTypeInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt64, vtkAOSDataArrayTemplate<vtkTypeInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
