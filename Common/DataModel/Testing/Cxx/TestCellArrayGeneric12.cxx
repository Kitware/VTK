// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpersSingleCellType2.h"
#include "vtkAOSDataArrayTemplate.h"

int TestCellArrayGeneric12(int, char*[])
{
  try
  {
    // clang-format off
    // Offsets: All integral types, Connectivity: vtkAOSDataArrayTemplate<All integral types>
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt8, vtkAOSDataArrayTemplate<vtkTypeUInt8>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt8, vtkAOSDataArrayTemplate<vtkTypeUInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt8, vtkAOSDataArrayTemplate<vtkTypeUInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt8, vtkAOSDataArrayTemplate<vtkTypeUInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt16, vtkAOSDataArrayTemplate<vtkTypeUInt8>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt16, vtkAOSDataArrayTemplate<vtkTypeUInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt16, vtkAOSDataArrayTemplate<vtkTypeUInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt16, vtkAOSDataArrayTemplate<vtkTypeUInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt32, vtkAOSDataArrayTemplate<vtkTypeUInt8>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt32, vtkAOSDataArrayTemplate<vtkTypeUInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt32, vtkAOSDataArrayTemplate<vtkTypeUInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt32, vtkAOSDataArrayTemplate<vtkTypeUInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt64, vtkAOSDataArrayTemplate<vtkTypeUInt8>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt64, vtkAOSDataArrayTemplate<vtkTypeUInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt64, vtkAOSDataArrayTemplate<vtkTypeUInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt64, vtkAOSDataArrayTemplate<vtkTypeUInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
