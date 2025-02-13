// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_5_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkTimerLog.h"

#include <cassert>
#include <iostream>

// undefine this to print benchmark results:
#define SILENT

int TestDataArrayIterators(int, char*[])
{
  vtkIdType numComps = 4;
  vtkIdType numValues = 100000000; // 10 million
  assert(numValues % numComps == 0);
  vtkIdType numTuples = numValues / numComps;

  vtkNew<vtkFloatArray> arrayContainer;
  vtkFloatArray* array = arrayContainer;
  array->SetNumberOfComponents(numComps);
  array->SetNumberOfTuples(numTuples);
  for (vtkIdType i = 0; i < numValues; ++i)
  {
    // Just fill with consistent data
    array->SetValue(i, i % 97);
  }

  // Create the vtkAOSDataArrayTemplate testing implementation:
  vtkNew<vtkAOSDataArrayTemplate<float>> tdaContainer;
  vtkAOSDataArrayTemplate<float>* tda = tdaContainer;
  tda->SetNumberOfComponents(numComps);
  tda->SetArray(arrayContainer->GetPointer(0), numValues, 1);

  // should be vtkAOSDataArrayTemplate<float>::Iterator (float*):
  vtkFloatArray::Iterator datBegin = array->Begin();
  vtkFloatArray::Iterator datIter = array->Begin();
  if (typeid(datBegin) != typeid(float*))
  {
    std::cerr << "Error: vtkFloatArray::Iterator is not a float*.";
    return EXIT_FAILURE;
  }

  // should be vtkTypedDataArrayIterator<float>:
  vtkAOSDataArrayTemplate<float>::Iterator tdaBegin =
    vtkAOSDataArrayTemplate<float>::FastDownCast(tda)->Begin();
  vtkAOSDataArrayTemplate<float>::Iterator tdaIter =
    vtkAOSDataArrayTemplate<float>::FastDownCast(tda)->Begin();
  if (typeid(tdaBegin) != typeid(float*))
  {
    std::cerr << "Error: vtkAOSDataArrayTemplate<float>::Iterator is not a "
                 "float*.";
    return EXIT_FAILURE;
  }

  // Validate that the iterators return the same values from operator[] and
  // operator* as GetValue;
  for (vtkIdType i = 0; i < numValues; ++i)
  {
    float lookup = array->GetValue(i);
    if (lookup != datBegin[i] || lookup != tdaBegin[i] || lookup != *datIter || lookup != *tdaIter)
    {
      std::cerr << "Mismatch at " << i << ":"
                << " GetValue(i)=" << lookup << " datBegin[i]=" << datBegin[i]
                << " tdaBegin[i]=" << tdaBegin[i] << " *datIter=" << *datIter
                << " *tdaIter=" << *tdaIter << std::endl;
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
    lookupSum += *array->GetPointer(i);
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

  // vtkAOSDataArrayTemplate<float>::Iterator:
  vtkAOSDataArrayTemplate<float>::Iterator tdaEnd = tda->End();
  float tdaSum = 0.f;
  timer->StartTimer();
  while (tdaBegin != tdaEnd)
  {
    tdaSum += *tdaBegin++;
  }
  timer->StopTimer();
  double tdaTime = timer->GetElapsedTime();

  std::cout << "GetValue time, sum: " << lookupTime << ", " << lookupSum << std::endl;
  std::cout << "dat time, sum:      " << datTime << ", " << datSum << std::endl;
  std::cout << "tda time, sum:      " << tdaTime << ", " << tdaSum << std::endl;
#endif

  return EXIT_SUCCESS;
}
