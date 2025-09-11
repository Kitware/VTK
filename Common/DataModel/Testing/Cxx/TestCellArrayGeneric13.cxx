// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestCellArrayHelpers.h"
#include "vtkSOADataArrayTemplate.h"

int TestCellArrayGeneric13(int, char*[])
{
  try
  {
    // clang-format off
    // Offsets: All integral types, Connectivity: vtkSOADataArrayTemplate<All integral types>
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt8>, vtkAffineArray<vtkTypeUInt8>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt16>, vtkAffineArray<vtkTypeUInt8>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt32>, vtkAffineArray<vtkTypeUInt8>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt64>, vtkAffineArray<vtkTypeUInt8>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt8>, vtkAffineArray<vtkTypeUInt16>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt16>, vtkAffineArray<vtkTypeUInt16>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt32>, vtkAffineArray<vtkTypeUInt16>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt64>, vtkAffineArray<vtkTypeUInt16>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt8>, vtkAffineArray<vtkTypeUInt32>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt16>, vtkAffineArray<vtkTypeUInt32>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt32>, vtkAffineArray<vtkTypeUInt32>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt64>, vtkAffineArray<vtkTypeUInt32>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt8>, vtkAffineArray<vtkTypeUInt64>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt16>, vtkAffineArray<vtkTypeUInt64>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt32>, vtkAffineArray<vtkTypeUInt64>>();
    ::RunTests<vtkCellArray::Generic, true, vtkSOADataArrayTemplate<vtkTypeUInt64>, vtkAffineArray<vtkTypeUInt64>>();
    // clang-format on
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
