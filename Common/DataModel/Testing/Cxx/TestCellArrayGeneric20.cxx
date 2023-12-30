// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpersSingleCellType2.h"
#include "vtkIdTypeArray.h"

int TestCellArrayGeneric20(int, char*[])
{
  try
  {
    // clang-format off
    // Offsets: All integral types, Connectivity: vtkIdTypeArray
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt8, vtkIdTypeArray>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt16, vtkIdTypeArray>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt32, vtkIdTypeArray>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellTypeSeparateDataTypes<vtkTypeUInt64, vtkIdTypeArray>(vtk::TakeSmartPointer(vtkCellArray::New()));
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
