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

#include <cassert>
#include <iostream>

// undefine this to print benchmark results:
#define SILENT

// Create a subclass of vtkTypedDataArray:
namespace
{
class MyArray : public vtkTypedDataArray<float>
{
  vtkFloatArray *Data;
public:
  vtkTypeMacro(MyArray, vtkTypedDataArray<float>)
  static MyArray *New() { VTK_STANDARD_NEW_BODY(MyArray) }
  void Init(vtkFloatArray *array)
  {
    this->Data = array;
    this->NumberOfComponents = array->GetNumberOfComponents();
    this->MaxId = array->GetMaxId();
  }
  ValueType& GetValueReference(vtkIdType idx) VTK_OVERRIDE
  {
    return *this->Data->GetPointer(idx);
  }

  // These pure virtuals are no-op -- all we care about is GetValueReference
  // to test the iterator.
  void SetTypedTuple(vtkIdType, const ValueType *) VTK_OVERRIDE {}
  void InsertTypedTuple(vtkIdType, const ValueType *) VTK_OVERRIDE {}
  vtkIdType InsertNextTypedTuple(const ValueType *) VTK_OVERRIDE { return 0; }
  vtkIdType LookupTypedValue(ValueType) VTK_OVERRIDE { return 0; }
  void LookupTypedValue(ValueType, vtkIdList*) VTK_OVERRIDE {}
  ValueType GetValue(vtkIdType) const VTK_OVERRIDE { return 0; }
  void SetValue(vtkIdType, ValueType) VTK_OVERRIDE {}
  void GetTypedTuple(vtkIdType, ValueType*) const VTK_OVERRIDE {}
  vtkIdType InsertNextValue(ValueType) VTK_OVERRIDE { return 0; }
  void InsertValue(vtkIdType, ValueType) VTK_OVERRIDE {}
  int Allocate(vtkIdType, vtkIdType) VTK_OVERRIDE { return 0; }
  int Resize(vtkIdType) VTK_OVERRIDE { return 0; }
};
}

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

  // Create the vtkTypedDataArray testing implementation:
  vtkNew<MyArray> tdaContainer;
  MyArray *tda = tdaContainer.GetPointer();
  tda->Init(array);

  // should be vtkAOSDataArrayTemplate<float>::Iterator (float*):
  vtkFloatArray::Iterator datBegin = array->Begin();
  vtkFloatArray::Iterator datIter = array->Begin();
  if (typeid(datBegin) != typeid(float*))
  {
    std::cerr << "Error: vtkFloatArray::Iterator is not a float*.";
    return EXIT_FAILURE;
  }

  // should be vtkTypedDataArrayIterator<float>:
  vtkTypedDataArray<float>::Iterator tdaBegin =
      vtkTypedDataArray<float>::FastDownCast(tda)->Begin();
  vtkTypedDataArray<float>::Iterator tdaIter =
      vtkTypedDataArray<float>::FastDownCast(tda)->Begin();
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

  // vtkTypedDataArrayIterator:
  vtkTypedDataArray<float>::Iterator tdaEnd = tda->End();
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
