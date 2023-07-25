// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

#define test_expression(expression)                                                                \
  do                                                                                               \
  {                                                                                                \
    if (!(expression))                                                                             \
    {                                                                                              \
      std::ostringstream buffer;                                                                   \
      buffer << "Expression failed at line " << __LINE__ << ": " << #expression;                   \
      throw std::runtime_error(buffer.str());                                                      \
    }                                                                                              \
  } while (false)

int TestArraySize(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  try
  {
    // Test to see that array sizes don't overflow ...
    vtkSmartPointer<vtkSparseArray<double>> array = vtkSmartPointer<vtkSparseArray<double>>::New();
    array->Resize(1200000, 18000);
    test_expression(array->GetSize() == 21600000000ULL);

    return 0;
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
