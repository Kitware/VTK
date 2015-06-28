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
  vtkWriteableGenericDataArrayMacro(array,
    for (ARRAY_TYPE::TupleIteratorType iter = ARRAY->Begin(), max = ARRAY->End(); iter != max; ++iter)
    {
    iter[0] = 1;
    iter[1] = 2;
    iter[2] = 3;
    }
  );
  array->Print(cout);
  array->Delete();
}

int TestGenericDataArray(int, char**)
{
  Test<vtkSoADataArrayTemplate<float> >();
  Test<vtkAoSDataArrayTemplate<float> >();
  return 1;
}
