// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>

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

int TestArrayVariants(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  try
  {
    // Exercise the API that gets/sets variants ...
    vtkSmartPointer<vtkDenseArray<double>> concrete = vtkSmartPointer<vtkDenseArray<double>>::New();
    concrete->Resize(3, 2);
    vtkTypedArray<double>* const typed = concrete;
    vtkArray* const abstract = concrete;

    abstract->SetVariantValue(0, 0, 1.0);
    abstract->SetVariantValue(vtkArrayCoordinates(0, 1), 2.0);
    typed->SetVariantValue(1, 0, 3.0);
    typed->SetVariantValue(vtkArrayCoordinates(1, 1), 4.0);
    concrete->SetVariantValue(2, 0, 5.0);
    concrete->SetVariantValue(vtkArrayCoordinates(2, 1), 6.0);

    test_expression(abstract->GetVariantValue(0, 0) == 1.0);
    test_expression(abstract->GetVariantValue(vtkArrayCoordinates(0, 1)) == 2.0);
    test_expression(typed->GetVariantValue(1, 0) == 3.0);
    test_expression(typed->GetVariantValue(vtkArrayCoordinates(1, 1)) == 4.0);
    test_expression(concrete->GetVariantValue(2, 0) == 5.0);
    test_expression(concrete->GetVariantValue(vtkArrayCoordinates(2, 1)) == 6.0);

    abstract->SetVariantValueN(0, 7.0);
    test_expression(abstract->GetVariantValueN(0) == 7.0);
    typed->SetVariantValueN(0, 8.0);
    test_expression(typed->GetVariantValueN(0) == 8.0);
    concrete->SetVariantValueN(0, 9.0);
    test_expression(concrete->GetVariantValueN(0) == 9.0);

    return 0;
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
