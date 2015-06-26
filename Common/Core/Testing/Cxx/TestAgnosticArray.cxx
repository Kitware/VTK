/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAgnosticArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkSoAArrayTemplate.h"

int TestAgnosticArray(int, char**)
{
  typedef vtkSoAArrayTemplate<float> vtkFloatArray2;
  vtkSoAArrayTemplate<float>* array = vtkSoAArrayTemplate<float>::New();
  array->SetNumberOfComponents(3);
  array->SetNumberOfTuples(100);
  vtkWriteableAgnosticArrayMacro(array,
    for (ARRAY_TYPE::TupleIteratorType iter = ARRAY->Begin(), max = ARRAY->End(); iter != max; ++iter)
    {
    iter[0] = 1;
    iter[1] = 2;
    iter[2] = 3;
    }
  );
  array->Print(cout);
  array->Delete();
  return 1;
}
