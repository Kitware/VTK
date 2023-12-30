// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpersSingleCellType2.h"
#include "vtkSOADataArrayTemplate.h"

int TestCellArrayGeneric16(int, char*[])
{
  try
  {
    // clang-format off
    // Offsets: All integral types, Connectivity: vtkSOADataArrayTemplate<All integral types>
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt8, vtkSOADataArrayTemplate<vtkTypeUInt8>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt8, vtkSOADataArrayTemplate<vtkTypeUInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt8, vtkSOADataArrayTemplate<vtkTypeUInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt8, vtkSOADataArrayTemplate<vtkTypeUInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt16, vtkSOADataArrayTemplate<vtkTypeUInt8>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt16, vtkSOADataArrayTemplate<vtkTypeUInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt16, vtkSOADataArrayTemplate<vtkTypeUInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt16, vtkSOADataArrayTemplate<vtkTypeUInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt32, vtkSOADataArrayTemplate<vtkTypeUInt8>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt32, vtkSOADataArrayTemplate<vtkTypeUInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt32, vtkSOADataArrayTemplate<vtkTypeUInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt32, vtkSOADataArrayTemplate<vtkTypeUInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt64, vtkSOADataArrayTemplate<vtkTypeUInt8>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt64, vtkSOADataArrayTemplate<vtkTypeUInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt64, vtkSOADataArrayTemplate<vtkTypeUInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt64, vtkSOADataArrayTemplate<vtkTypeUInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
