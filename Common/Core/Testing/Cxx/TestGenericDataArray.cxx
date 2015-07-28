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


template <class T>
void Test()
{
  T* array = T::New();
  array->SetNumberOfComponents(3);
  array->SetNumberOfTuples(100);
  vtkGenericDataArrayMacro(array,
    for (vtkIdType tupleIdx=0, max=ARRAY->GetNumberOfTuples(); tupleIdx < max;
         ++tupleIdx)
    {
    ARRAY->SetComponentValue(tupleIdx, 0, 1);
    ARRAY->SetComponentValue(tupleIdx, 1, 2);
    ARRAY->SetComponentValue(tupleIdx, 2, 3);
    }
  );
  array->Print(cout);
  array->Delete();
}

int TestGenericDataArray(int, char**)
{
  Test<vtkSoADataArrayTemplate<float> >();
  Test<vtkAoSDataArrayTemplate<float> >();
  return EXIT_SUCCESS;
}
