// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpersSingleCellType2.h"
#include "vtkAOSDataArrayTemplate.h"

int TestCellArrayGeneric14(int, char*[])
{
  try
  {
    // clang-format off
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt16, vtkAOSDataArrayTemplate<vtkTypeUInt8>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt16, vtkAOSDataArrayTemplate<vtkTypeUInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt16, vtkAOSDataArrayTemplate<vtkTypeUInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt16, vtkAOSDataArrayTemplate<vtkTypeUInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt32, vtkAOSDataArrayTemplate<vtkTypeUInt8>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt32, vtkAOSDataArrayTemplate<vtkTypeUInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt32, vtkAOSDataArrayTemplate<vtkTypeUInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt32, vtkAOSDataArrayTemplate<vtkTypeUInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt64, vtkAOSDataArrayTemplate<vtkTypeUInt8>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt64, vtkAOSDataArrayTemplate<vtkTypeUInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt64, vtkAOSDataArrayTemplate<vtkTypeUInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeInt64, vtkAOSDataArrayTemplate<vtkTypeUInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
