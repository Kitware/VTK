/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestArrayLookup.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBitArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkSortDataArray.h"
#include "vtkStringArray.h"
#include "vtkTimerLog.h"
#include "vtkVariantArray.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <algorithm>
#include <limits>
#include <map>
#include <utility>
#include <vector>


struct NodeCompare
{
  bool operator()(const std::pair<int, vtkIdType>& a, const std::pair<int, vtkIdType>& b) const
  {
    return a.first < b.first;
  }
};

vtkIdType LookupValue(std::multimap<int, vtkIdType>& lookup, int value)
{
  std::pair<const int, vtkIdType> found = *lookup.lower_bound(value);
  if (found.first == value)
  {
    return found.second;
  }
  return -1;
}

vtkIdType LookupValue(std::vector<std::pair<int, vtkIdType> >& lookup, int value)
{
  NodeCompare comp;
  std::pair<int, vtkIdType> val(value, 0);
  std::pair<int, vtkIdType> found =
    *std::lower_bound(lookup.begin(), lookup.end(), val, comp);
  if (found.first == value)
  {
    return found.second;
  }
  return -1;
}

vtkIdType LookupValue(vtkIntArray* lookup, vtkIdTypeArray* index, int value)
{
  int* ptr = lookup->GetPointer(0);
  vtkIdType place = static_cast<vtkIdType>(std::lower_bound(ptr, ptr + lookup->GetNumberOfTuples(), value) - ptr);
  if (place < lookup->GetNumberOfTuples() && ptr[place] == value)
  {
    return index->GetValue(place);
  }
  return -1;
}

int TestArrayLookupBit(vtkIdType numVal)
{
  int errors = 0;

  // Create the array
  vtkIdType arrSize = (numVal-1)*numVal/2;
  VTK_CREATE(vtkBitArray, arr);
  for (vtkIdType i = 0; i < arrSize; i++)
  {
    arr->InsertNextValue(i < arrSize/2);
  }

  //
  // Test lookup implemented inside data array
  //

  // Time the lookup creation
  VTK_CREATE(vtkTimerLog, timer);
  timer->StartTimer();
  arr->LookupValue(0);
  timer->StopTimer();
  cerr << "," << timer->GetElapsedTime();

  // Time simple lookup
  timer->StartTimer();
  for (vtkIdType i = 0; i < numVal; i++)
  {
    arr->LookupValue(i % 2);
  }
  timer->StopTimer();
  cerr << "," << (timer->GetElapsedTime() / static_cast<double>(numVal));

  // Time list lookup
  VTK_CREATE(vtkIdList, list);
  timer->StartTimer();
  for (vtkIdType i = 0; i < numVal; i++)
  {
    arr->LookupValue(i % 2, list);
  }
  timer->StopTimer();
  cerr << "," << (timer->GetElapsedTime() / static_cast<double>(numVal));

  // Test for correctness (-1)
  vtkIdType index = -1;
  index = arr->LookupValue(-1);
  if (index != -1)
  {
    cerr << "ERROR: lookup found value at " << index << " but is not there (should return -1)" << endl;
    errors++;
  }
  arr->LookupValue(-1, list);
  if (list->GetNumberOfIds() != 0)
  {
    cerr << "ERROR: lookup found " << list->GetNumberOfIds() << " matches but there should be " << 0 << endl;
    errors++;
  }

  // Test for correctness (0)
  index = arr->LookupValue(0);
  if (index < arrSize/2 || index > arrSize-1)
  {
    cerr << "ERROR: vector lookup found value at " << index << " but is in range [" << arrSize/2 << "," << arrSize-1 << "]" << endl;
    errors++;
  }
  arr->LookupValue(0, list);
  if (list->GetNumberOfIds() != arrSize - arrSize/2)
  {
    cerr << "ERROR: lookup found " << list->GetNumberOfIds() << " matches but there should be " << arrSize - arrSize/2 << endl;
    errors++;
  }
  else
  {
    for (vtkIdType j = 0; j < list->GetNumberOfIds(); j++)
    {
      if (arr->GetValue(list->GetId(j)) != 0)
      {
        cerr << "ERROR: could not find " << j << " in found list" << endl;
        errors++;
      }
    }
  }

  // Test for correctness (1)
  index = arr->LookupValue(1);
  if (index < 0 || index > arrSize/2-1)
  {
    cerr << "ERROR: vector lookup found value at " << index << " but is in range [" << 0 << "," << arrSize/2-1 << "]" << endl;
    errors++;
  }
  arr->LookupValue(1, list);
  if (list->GetNumberOfIds() != arrSize/2)
  {
    cerr << "ERROR: lookup found " << list->GetNumberOfIds() << " matches but there should be " << arrSize/2 << endl;
    errors++;
  }
  else
  {
    for (vtkIdType j = 0; j < list->GetNumberOfIds(); j++)
    {
      if (arr->GetValue(list->GetId(j)) != 1)
      {
        cerr << "ERROR: could not find " << j << " in found list" << endl;
        errors++;
      }
    }
  }

  return errors;
}

int TestArrayLookupVariant(vtkIdType numVal)
{
  int errors = 0;

  // Create the array
  vtkIdType arrSize = (numVal-1)*numVal/2;
  VTK_CREATE(vtkVariantArray, arr);
  for (vtkIdType i = 0; i < numVal; i++)
  {
    for (vtkIdType j = 0; j < numVal-1-i; j++)
    {
      arr->InsertNextValue(numVal-1-i);
    }
  }

  //
  // Test lookup implemented inside data array
  //

  // Time the lookup creation
  VTK_CREATE(vtkTimerLog, timer);
  timer->StartTimer();
  arr->LookupValue(0);
  timer->StopTimer();
  cerr << "," << timer->GetElapsedTime();

  // Time simple lookup
  timer->StartTimer();
  for (vtkIdType i = 0; i < numVal; i++)
  {
    arr->LookupValue(i);
  }
  timer->StopTimer();
  cerr << "," << (timer->GetElapsedTime() / static_cast<double>(numVal));

  // Time list lookup
  VTK_CREATE(vtkIdList, list);
  timer->StartTimer();
  for (vtkIdType i = 0; i < numVal; i++)
  {
    arr->LookupValue(i, list);
  }
  timer->StopTimer();
  cerr << "," << (timer->GetElapsedTime() / static_cast<double>(numVal));

  // Test for correctness
  vtkIdType correctIndex = arrSize;
  for (vtkIdType i = 0; i < numVal; i++)
  {
    correctIndex -= i;
    vtkIdType index = arr->LookupValue(i);
    if (i == 0 && index != -1)
    {
      cerr << "ERROR: lookup found value at " << index << " but is at -1" << endl;
      errors++;
    }
    if (i != 0 && (index < correctIndex || index > correctIndex + i - 1))
    {
      cerr << "ERROR: vector lookup found value at " << index << " but is in range [" << correctIndex << "," << correctIndex + i - 1 << "]" << endl;
      errors++;
    }
    arr->LookupValue(i, list);
    if (list->GetNumberOfIds() != i)
    {
      cerr << "ERROR: lookup found " << list->GetNumberOfIds() << " matches but there should be " << i << endl;
      errors++;
    }
    else
    {
      for (vtkIdType j = correctIndex; j < correctIndex + i; j++)
      {
        bool inList = false;
        for (vtkIdType k = 0; k < i; ++k)
        {
          if (list->GetId(k) == j)
          {
            inList = true;
            break;
          }
        }
        if (!inList)
        {
          cerr << "ERROR: could not find " << j << " in found list" << endl;
          errors++;
        }
      }
    }
  }
  return errors;
}

int TestArrayLookupFloat(vtkIdType numVal)
{
  int errors = 0;

  // Create the array
  vtkIdType arrSize = (numVal-1)*numVal/2;
  VTK_CREATE(vtkFloatArray, arr);
  for (vtkIdType i = 0; i < numVal; i++)
  {
    for (vtkIdType j = 0; j < numVal-1-i; j++)
    {
      arr->InsertNextValue(numVal-1-i);
    }
  }
  arr->InsertNextValue(std::numeric_limits<float>::quiet_NaN());

  //
  // Test lookup implemented inside data array
  //

  // Time the lookup creation
  VTK_CREATE(vtkTimerLog, timer);
  timer->StartTimer();
  arr->LookupValue(0);
  timer->StopTimer();
  cerr << "," << timer->GetElapsedTime();

  // Time simple lookup
  timer->StartTimer();
  for (vtkIdType i = 0; i < numVal; i++)
  {
    arr->LookupValue(i);
  }
  timer->StopTimer();
  cerr << "," << (timer->GetElapsedTime() / static_cast<double>(numVal));

  // Time list lookup
  VTK_CREATE(vtkIdList, list);
  timer->StartTimer();
  for (vtkIdType i = 0; i < numVal; i++)
  {
    arr->LookupValue(i, list);
  }
  timer->StopTimer();
  cerr << "," << (timer->GetElapsedTime() / static_cast<double>(numVal));

  // Test for NaN
  {
    vtkIdType index = arr->LookupValue(std::numeric_limits<float>::quiet_NaN());
    if (index != arrSize) {
      cerr << "ERROR: lookup found NaN at " << index << " instead of " << arrSize << endl;
      errors++;
    }
  }
  {
    vtkNew<vtkIdList> NaNlist;
    arr->LookupValue(std::numeric_limits<float>::quiet_NaN(), NaNlist.Get());
    if (NaNlist->GetNumberOfIds() != 1)
    {
      cerr << "ERROR: lookup found " << list->GetNumberOfIds() << " values of NaN instead of " << 1 << endl;
      errors++;
    }
    if (NaNlist->GetId(0) != arrSize)
    {
      cerr << "ERROR: lookup found NaN at " << list->GetId(0) << " instead of " << arrSize << endl;
      errors++;
    }
  }

  // Test for correctness
  vtkIdType correctIndex = arrSize;
  for (vtkIdType i = 0; i < numVal; i++)
  {
    correctIndex -= i;
    vtkIdType index = arr->LookupValue(i);
    if (i == 0 && index != -1)
    {
      cerr << "ERROR: lookup found value at " << index << " but is at -1" << endl;
      errors++;
    }
    if (i != 0 && (index < correctIndex || index > correctIndex + i - 1))
    {
      cerr << "ERROR: vector lookup found value at " << index << " but is in range [" << correctIndex << "," << correctIndex + i - 1 << "]" << endl;
      errors++;
    }
    arr->LookupValue(i, list);
    if (list->GetNumberOfIds() != i)
    {
      cerr << "ERROR: lookup found " << list->GetNumberOfIds() << " matches but there should be " << i << endl;
      errors++;
    }
    else
    {
      for (vtkIdType j = correctIndex; j < correctIndex + i; j++)
      {
        bool inList = false;
        for (vtkIdType k = 0; k < i; ++k)
        {
          if (list->GetId(k) == j)
          {
            inList = true;
            break;
          }
        }
        if (!inList)
        {
          cerr << "ERROR: could not find " << j << " in found list" << endl;
          errors++;
        }
      }
    }
  }
  return errors;
}


int TestArrayLookupString(vtkIdType numVal)
{
  int errors = 0;

  // Create the array
  vtkIdType arrSize = (numVal-1)*numVal/2;
  VTK_CREATE(vtkStringArray, arr);
  for (vtkIdType i = 0; i < numVal; i++)
  {
    for (vtkIdType j = 0; j < numVal-1-i; j++)
    {
      arr->InsertNextValue(vtkVariant(numVal-1-i).ToString());
    }
  }

  //
  // Test lookup implemented inside data array
  //

  // Time the lookup creation
  VTK_CREATE(vtkTimerLog, timer);
  timer->StartTimer();
  arr->LookupValue("0");
  timer->StopTimer();
  cerr << "," << timer->GetElapsedTime();

  // Time simple lookup
  timer->StartTimer();
  for (vtkIdType i = 0; i < numVal; i++)
  {
    arr->LookupValue(vtkVariant(i).ToString());
  }
  timer->StopTimer();
  cerr << "," << (timer->GetElapsedTime() / static_cast<double>(numVal));

  // Time list lookup
  VTK_CREATE(vtkIdList, list);
  timer->StartTimer();
  for (vtkIdType i = 0; i < numVal; i++)
  {
    arr->LookupValue(vtkVariant(i).ToString(), list);
  }
  timer->StopTimer();
  cerr << "," << (timer->GetElapsedTime() / static_cast<double>(numVal));

  // Test for correctness
  vtkIdType correctIndex = arrSize;
  for (vtkIdType i = 0; i < numVal; i++)
  {
    correctIndex -= i;
    vtkIdType index = arr->LookupValue(vtkVariant(i).ToString());
    if (i == 0 && index != -1)
    {
      cerr << "ERROR: lookup found value at " << index << " but is at -1" << endl;
      errors++;
    }
    if (i != 0 && (index < correctIndex || index > correctIndex + i - 1))
    {
      cerr << "ERROR: vector lookup found value at " << index << " but is in range [" << correctIndex << "," << correctIndex + i - 1 << "]" << endl;
      errors++;
    }
    arr->LookupValue(vtkVariant(i).ToString(), list);
    if (list->GetNumberOfIds() != i)
    {
      cerr << "ERROR: lookup found " << list->GetNumberOfIds() << " matches but there should be " << i << endl;
      errors++;
    }
    else
    {
      for (vtkIdType j = correctIndex; j < correctIndex + i; j++)
      {
        bool inList = false;
        for (vtkIdType k = 0; k < i; ++k)
        {
          if (list->GetId(k) == j)
          {
            inList = true;
            break;
          }
        }
        if (!inList)
        {
          cerr << "ERROR: could not find " << j << " in found list" << endl;
          errors++;
        }
      }
    }
  }
  return errors;
}

int TestArrayLookupInt(vtkIdType numVal, bool runComparison)
{
  int errors = 0;

  // Create the array
  vtkIdType arrSize = (numVal-1)*numVal/2;
  VTK_CREATE(vtkIntArray, arr);
  for (vtkIdType i = 0; i < numVal; i++)
  {
    for (vtkIdType j = 0; j < numVal-1-i; j++)
    {
      arr->InsertNextValue(numVal-1-i);
    }
  }

  //
  // Test lookup implemented inside data array
  //

  // Time the lookup creation
  VTK_CREATE(vtkTimerLog, timer);
  timer->StartTimer();
  arr->LookupValue(0);
  timer->StopTimer();
  cerr << "," << timer->GetElapsedTime();

  // Time simple lookup
  timer->StartTimer();
  for (vtkIdType i = 0; i < numVal; i++)
  {
    arr->LookupValue(i);
  }
  timer->StopTimer();
  cerr << "," << (timer->GetElapsedTime() / static_cast<double>(numVal));

  // Time list lookup
  VTK_CREATE(vtkIdList, list);
  timer->StartTimer();
  for (vtkIdType i = 0; i < numVal; i++)
  {
    arr->LookupValue(i, list);
  }
  timer->StopTimer();
  cerr << "," << (timer->GetElapsedTime() / static_cast<double>(numVal));

  // Test for correctness
  vtkIdType correctIndex = arrSize;
  for (vtkIdType i = 0; i < numVal; i++)
  {
    correctIndex -= i;
    vtkIdType index = arr->LookupValue(i);
    if (i == 0 && index != -1)
    {
      cerr << "ERROR: lookup found value at " << index << " but is at -1" << endl;
      errors++;
    }
    if (i != 0 && (index < correctIndex || index > correctIndex + i - 1))
    {
      cerr << "ERROR: vector lookup found value at " << index << " but is in range [" << correctIndex << "," << correctIndex + i - 1 << "]" << endl;
      errors++;
    }
    arr->LookupValue(i, list);
    if (list->GetNumberOfIds() != i)
    {
      cerr << "ERROR: lookup found " << list->GetNumberOfIds() << " matches but there should be " << i << endl;
      errors++;
    }
    else
    {
      for (vtkIdType j = correctIndex; j < correctIndex + i; j++)
      {
        bool inList = false;
        for (vtkIdType k = 0; k < i; ++k)
        {
          if (list->GetId(k) == j)
          {
            inList = true;
            break;
          }
        }
        if (!inList)
        {
          cerr << "ERROR: could not find " << j << " in found list" << endl;
          errors++;
        }
      }
    }
  }

  if (runComparison)
  {
    //
    // Test STL map lookup
    //

    // Time the lookup creation
    timer->StartTimer();
    int* ptr = arr->GetPointer(0);
    std::multimap<int, vtkIdType> map;
    for (vtkIdType i = 0; i < arrSize; ++i, ++ptr)
    {
      map.insert(std::pair<const int, vtkIdType>(*ptr, i));
    }
    timer->StopTimer();
    cerr << "," << timer->GetElapsedTime();

    // Time simple lookup
    timer->StartTimer();
    for (vtkIdType i = 0; i < numVal; i++)
    {
      LookupValue(map, i);
    }
    timer->StopTimer();
    cerr << "," << (timer->GetElapsedTime() / static_cast<double>(numVal));

    // Test for correctness
    correctIndex = arrSize;
    for (vtkIdType i = 0; i < numVal; i++)
    {
      correctIndex -= i;
      vtkIdType index = LookupValue(map, i);
      if (i == 0 && index != -1)
      {
        cerr << "ERROR: lookup found value at " << index << " but is at -1" << endl;
        errors++;
      }
      if (i != 0 && index != correctIndex)
      {
        cerr << "ERROR: lookup found value at " << index << " but is at " << correctIndex << endl;
        errors++;
      }
    }

    //
    // Test STL vector lookup
    //

    // Time lookup creation
    timer->StartTimer();
    ptr = arr->GetPointer(0);
    std::vector<std::pair<int, vtkIdType> > vec(arrSize);
    for (vtkIdType i = 0; i < arrSize; ++i, ++ptr)
    {
      vec[i] = std::pair<int, vtkIdType>(*ptr, i);
    }
    NodeCompare comp;
    std::sort(vec.begin(), vec.end(), comp);
    timer->StopTimer();
    cerr << "," << timer->GetElapsedTime();

    // Time simple lookup
    timer->StartTimer();
    for (vtkIdType i = 0; i < numVal; i++)
    {
      LookupValue(vec, i);
    }
    timer->StopTimer();
    cerr << "," << (timer->GetElapsedTime() / static_cast<double>(numVal));

    // Test for correctness
    correctIndex = arrSize;
    for (vtkIdType i = 0; i < numVal; i++)
    {
      correctIndex -= i;
      vtkIdType index = LookupValue(vec, i);
      if (i == 0 && index != -1)
      {
        cerr << "ERROR: vector lookup found value at " << index << " but is at -1" << endl;
        errors++;
      }
      if (i != 0 && (index < correctIndex || index > correctIndex + i - 1))
      {
        cerr << "ERROR: vector lookup found value at " << index << " but is in range [" << correctIndex << "," << correctIndex + i - 1 << "]" << endl;
        errors++;
      }
    }

    //
    // Test sorted data array lookup
    //

    // Time lookup creation
    timer->StartTimer();
    VTK_CREATE(vtkIdTypeArray, indices);
    indices->SetNumberOfTuples(arrSize);
    vtkIdType* keyptr = indices->GetPointer(0);
    for (vtkIdType i = 0; i < arrSize; ++i, ++keyptr)
    {
      *keyptr = i;
    }
    VTK_CREATE(vtkIntArray, sorted);
    sorted->DeepCopy(arr);
    vtkSortDataArray::Sort(sorted, indices);
    timer->StopTimer();
    cerr << "," << timer->GetElapsedTime();

    // Time simple lookup
    timer->StartTimer();
    for (vtkIdType i = 0; i < numVal; i++)
    {
      LookupValue(sorted, indices, i);
    }
    timer->StopTimer();
    cerr << "," << (timer->GetElapsedTime() / static_cast<double>(numVal));

    // Test for correctness
    correctIndex = arrSize;
    for (vtkIdType i = 0; i < numVal; i++)
    {
      correctIndex -= i;
      vtkIdType index = LookupValue(sorted, indices, i);
      if (i == 0 && index != -1)
      {
        cerr << "ERROR: arr lookup found value at " << index << " but is at -1" << endl;
        errors++;
      }
      if (i != 0 && (index < correctIndex || index > correctIndex + i - 1))
      {
        cerr << "ERROR: arr lookup found value at " << index << " but is in range [" << correctIndex << "," << correctIndex + i - 1 << "]" << endl;
        errors++;
      }
    }
  }

  return errors;
}

int TestArrayLookup(int argc, char* argv[])
{
  vtkIdType min = 100;
  vtkIdType max = 200;
  int steps = 2;
  bool runComparison = false;
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "-C"))
    {
      runComparison = true;
    }
    if (!strcmp(argv[i], "-m") && i+1 < argc)
    {
      ++i;
      int size = atoi(argv[i]);
      min = static_cast<int>((-1.0 + sqrt(1 + 8.0*size))/2.0);
    }
    if (!strcmp(argv[i], "-M") && i+1 < argc)
    {
      ++i;
      int size = atoi(argv[i]);
      max = static_cast<int>((-1.0 + sqrt(1 + 8.0*size))/2.0);
    }
    if (!strcmp(argv[i], "-S") && i+1 < argc)
    {
      ++i;
      steps = atoi(argv[i]);
    }
  }

  vtkIdType stepSize = (max-min)/(steps-1);
  if (stepSize <= 0)
  {
    stepSize = 1;
  }

  int errors = 0;
  cerr << "distinct values";
  cerr << ",size";
  cerr << ",create lookup";
  cerr << ",index lookup";
  cerr << ",list lookup";
  if (runComparison)
  {
    cerr << ",create map lookup";
    cerr << ",index map lookup";
    cerr << ",create vector lookup";
    cerr << ",index vector lookup";
    cerr << ",create array lookup";
    cerr << ",index array lookup";
  }
  cerr << ",string create lookup";
  cerr << ",string index lookup";
  cerr << ",string list lookup";
  cerr << ",variant create lookup";
  cerr << ",variant index lookup";
  cerr << ",variant list lookup";
  cerr << ",bit create lookup";
  cerr << ",bit index lookup";
  cerr << ",bit list lookup";
  cerr << endl;
  for (vtkIdType numVal = min; numVal <= max; numVal += stepSize)
  {
    vtkIdType total = numVal*(numVal+1)/2;
    cerr << numVal << "," << total;
    errors += TestArrayLookupInt(numVal, runComparison);
    errors += TestArrayLookupFloat(numVal);
    errors += TestArrayLookupString(numVal);
    errors += TestArrayLookupVariant(numVal);
    errors += TestArrayLookupBit(numVal);
    cerr << endl;
  }
  return errors;
}
