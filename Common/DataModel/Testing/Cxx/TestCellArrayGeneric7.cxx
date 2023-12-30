// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpersSingleCellType1.h"
#include "vtkSOADataArrayTemplate.h"

int TestCellArrayGeneric7(int, char*[])
{
  try
  {
    // clang-format off
    // Connectivity: vtkSOADataArrayTemplate<All integral types>
    TestSetDataSingleCellType<vtkSOADataArrayTemplate<vtkTypeUInt8>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<vtkSOADataArrayTemplate<vtkTypeUInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<vtkSOADataArrayTemplate<vtkTypeUInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<vtkSOADataArrayTemplate<vtkTypeUInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<vtkSOADataArrayTemplate<vtkTypeInt16>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<vtkSOADataArrayTemplate<vtkTypeInt32>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<vtkSOADataArrayTemplate<vtkTypeInt64>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    TestSetDataSingleCellType<vtkSOADataArrayTemplate<vtkIdType>>(vtk::TakeSmartPointer(vtkCellArray::New()));
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
