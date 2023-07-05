// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include <vtkArrayData.h>
#include <vtkArrayPrint.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>
#include <vtkTransposeMatrix.h>

#include <iostream>
#include <stdexcept>

#define test_expression(expression)                                                                \
  do                                                                                               \
  {                                                                                                \
    if (!(expression))                                                                             \
      throw std::runtime_error("Expression failed: " #expression);                                 \
  } while (false)

int ArrayTransposeMatrix(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  cout << setprecision(17);

  try
  {
    vtkSmartPointer<vtkSparseArray<double>> source = vtkSmartPointer<vtkSparseArray<double>>::New();
    source->Resize(vtkArrayExtents(3, 2));
    source->AddValue(vtkArrayCoordinates(0, 1), 1);
    source->AddValue(vtkArrayCoordinates(1, 0), 2);
    source->AddValue(vtkArrayCoordinates(2, 0), 3);

    cout << "source matrix:\n";
    vtkPrintMatrixFormat(cout, source.GetPointer());

    vtkSmartPointer<vtkArrayData> source_data = vtkSmartPointer<vtkArrayData>::New();
    source_data->AddArray(source);

    vtkSmartPointer<vtkTransposeMatrix> transpose = vtkSmartPointer<vtkTransposeMatrix>::New();
    transpose->SetInputData(source_data);
    transpose->Update();

    vtkSparseArray<double>* const output = vtkSparseArray<double>::SafeDownCast(
      transpose->GetOutput()->GetArray(static_cast<vtkIdType>(0)));
    cout << "output matrix:\n";
    vtkPrintMatrixFormat(cout, output);

    test_expression(output);
    test_expression(output->GetExtent(0).GetSize() == 2);
    test_expression(output->GetExtent(1).GetSize() == 3);

    test_expression(output->GetValue(vtkArrayCoordinates(0, 0)) == 0);
    test_expression(output->GetValue(vtkArrayCoordinates(0, 1)) == 2);
    test_expression(output->GetValue(vtkArrayCoordinates(0, 2)) == 3);
    test_expression(output->GetValue(vtkArrayCoordinates(1, 0)) == 1);
    test_expression(output->GetValue(vtkArrayCoordinates(1, 1)) == 0);
    test_expression(output->GetValue(vtkArrayCoordinates(1, 2)) == 0);

    return 0;
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
