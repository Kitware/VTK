/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataArrayIterators.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFloatArray.h"
#include "vtkTimerLog.h"
#include "vtkTypedDataArray.h"
#include "vtkTypedDataArrayIterator.h"
#include "vtkNew.h"

#include <assert.h>
#include <iostream>

// undefine this to print benchmark results:
#define SILENT

int TestDataArrayIterators(int, char *[])
{
  vtkIdType numComps = 4;
  vtkIdType numValues = 100000000; // 10 million
  assert(numValues % numComps == 0);
  vtkIdType numTuples = numValues / numComps;

  vtkNew<vtkFloatArray> arrayContainer;
  vtkFloatArray *array = arrayContainer.GetPointer();
  array->SetNumberOfComponents(numComps);
  array->SetNumberOfTuples(numTuples);
  for (vtkIdType i = 0; i < numValues; ++i)
    {
    // Just fill with consistent data
    array->SetValue(i, i % 97);
    }

  // should be vtkDataArrayTemplate<float>::Iterator (float*):
  vtkFloatArray::Iterator datBegin = array->Begin();
  vtkFloatArray::Iterator datIter = array->Begin();
  if (typeid(datBegin) != typeid(float*))
    {
    std::cerr << "Error: vtkFloatArray::Iterator is not a float*.";
    return EXIT_FAILURE;
    }

  // should be vtkTypedDataArrayIterator<float>:
  vtkTypedDataArray<float>::Iterator tdaBegin =
      vtkTypedDataArray<float>::FastDownCast(array)->Begin();
  vtkTypedDataArray<float>::Iterator tdaIter =
      vtkTypedDataArray<float>::FastDownCast(array)->Begin();
  if (typeid(tdaBegin) != typeid(vtkTypedDataArrayIterator<float>))
    {
    std::cerr << "Error: vtkTypedDataArray<float>::Iterator is not a "
                 "vtkTypedDataArrayIterator<float>.";
    return EXIT_FAILURE;
    }

  // Validate that the iterators return the same values from operator[] and
  // operator* as GetValue;
  for (vtkIdType i = 0; i < numValues; ++i)
    {
    float lookup = array->GetValue(i);
    if (lookup != datBegin[i] || lookup != tdaBegin[i] ||
        lookup != *datIter    || lookup != *tdaIter)
      {
      std::cerr << "Mismatch at " << i << ":"
                << " GetValue(i)=" << lookup
                << " datBegin[i]=" << datBegin[i]
                << " tdaBegin[i]=" << tdaBegin[i]
                << " *datIter=" << *datIter
                << " *tdaIter=" << *tdaIter
                << std::endl;
      return EXIT_FAILURE;
      }
    ++datIter;
    ++tdaIter;
    }

#ifndef SILENT
  // Iterator timings.
  vtkNew<vtkTimerLog> timer;

  // Lookup:
  float lookupSum = 0.f;
  timer->StartTimer();
  for (vtkIdType i = 0; i < numValues; ++i)
    {
    lookupSum += array->GetValueReference(i);
    }
  timer->StopTimer();
  double lookupTime = timer->GetElapsedTime();

  // Scalar iterator:
  float datSum = 0.f;
  timer->StartTimer();
  vtkFloatArray::Iterator datEnd = array->End();
  while (datBegin != datEnd)
    {
    datSum += *datBegin++;
    }
  timer->StopTimer();
  double datTime = timer->GetElapsedTime();

  // vtkTypedDataArrayIterator:
  vtkTypedDataArray<float>::Iterator tdaEnd =
      vtkTypedDataArray<float>::FastDownCast(array)->End();
  float tdaSum = 0.f;
  timer->StartTimer();
  while (tdaBegin != tdaEnd)
    {
    tdaSum += *tdaBegin++;
    }
  timer->StopTimer();
  double tdaTime = timer->GetElapsedTime();

  std::cout << "GetValue time, sum: "
            << lookupTime << ", " << lookupSum << std::endl;
  std::cout << "dat time, sum:      "
            << datTime << ", " << datSum << std::endl;
  std::cout << "tda time, sum:      "
            << tdaTime << ", " << tdaSum << std::endl;
#endif

  return EXIT_SUCCESS;
}
