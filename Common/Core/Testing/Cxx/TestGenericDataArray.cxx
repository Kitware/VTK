/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericDataArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkSoADataArrayTemplate.h"
#include "vtkAoSDataArrayTemplate.h"

#include "vtkArrayDispatch.h"

struct TestWorker
{
  template <typename Array1T>
  void operator()(Array1T *array)
  {
    for (vtkIdType tupleIdx = 0, max = array->GetNumberOfTuples();
         tupleIdx < max;
         ++tupleIdx)
    {
    array->SetComponentValue(tupleIdx, 0, 1);
    array->SetComponentValue(tupleIdx, 1, 2);
    array->SetComponentValue(tupleIdx, 2, 3);
    }
  }
};

template <class T>
void Test()
{
  T* array = T::New();
  array->SetNumberOfComponents(3);
  array->SetNumberOfTuples(100);

  TestWorker worker;
  if (!vtkArrayDispatch::Dispatch::Execute(array, worker))
    {
    std::cerr << "Dispatch failed! Array: " << array->GetClassName()
              << std::endl;
    }
  else
    {
    array->Print(cout);
    }

  array->Delete();
}

int TestGenericDataArray(int, char**)
{
  Test<vtkSoADataArrayTemplate<float> >();
  Test<vtkAoSDataArrayTemplate<float> >();
  return EXIT_SUCCESS;
}
