// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2004 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkSortDataArray.h"
#include "vtkStringArray.h"
#include "vtkTimerLog.h"

#include <locale> // C++ locale
#include <sstream>

#include <iostream>

// #define ARRAY_SIZE (2*1024*1024)
#define ARRAY_SIZE 2048

int TestSortDataArray(int, char*[])
{
  vtkIdType i;
  vtkTimerLog* timer = vtkTimerLog::New();
  int retVal = 0;

  //---------------------------------------------------------------------------
  // Sort data array
  std::cout << "Building array----------" << std::endl;
  vtkIntArray* keys = vtkIntArray::New();
  keys->SetNumberOfComponents(1);
  keys->SetNumberOfTuples(ARRAY_SIZE);
  for (i = 0; i < ARRAY_SIZE; i++)
  {
    keys->SetComponent(i, 0, static_cast<int>(vtkMath::Random(0, ARRAY_SIZE * 4)));
  }

  std::cout << "Sorting array" << std::endl;
  timer->StartTimer();
  vtkSortDataArray::Sort(keys);
  timer->StopTimer();

  std::cout << "Time to sort array: " << timer->GetElapsedTime() << " sec" << std::endl;

  for (i = 0; i < ARRAY_SIZE - 1; i++)
  {
    if (keys->GetComponent(i, 0) > keys->GetComponent(i + 1, 0))
    {
      std::cout << "Array not properly sorted!" << std::endl;
      retVal = 1;
      break;
    }
  }
  std::cout << "Array consistency check finished\n" << std::endl;

  std::cout << "Sorting sorted array" << std::endl;
  timer->StartTimer();
  vtkSortDataArray::Sort(keys);
  timer->StopTimer();

  std::cout << "Time to sort array: " << timer->GetElapsedTime() << " sec" << std::endl;

  for (i = 0; i < ARRAY_SIZE - 1; i++)
  {
    if (keys->GetComponent(i, 0) > keys->GetComponent(i + 1, 0))
    {
      std::cout << "Array not properly sorted!" << std::endl;
      retVal = 1;
      break;
    }
  }
  std::cout << "Array consistency check finished\n" << std::endl;

  //---------------------------------------------------------------------------
  // Sort id list (ascending)
  std::cout << "Building id list (ascending order)----------" << std::endl;
  vtkIdList* ids = vtkIdList::New();
  ids->SetNumberOfIds(ARRAY_SIZE);
  for (i = 0; i < ARRAY_SIZE; i++)
  {
    ids->SetId(i, static_cast<vtkIdType>(vtkMath::Random(0, ARRAY_SIZE * 4)));
  }

  std::cout << "Sorting ids" << std::endl;
  timer->StartTimer();
  vtkSortDataArray::Sort(ids);
  timer->StopTimer();

  std::cout << "Time to sort ids: " << timer->GetElapsedTime() << " sec" << std::endl;

  for (i = 0; i < ARRAY_SIZE - 1; i++)
  {
    if (ids->GetId(i) > ids->GetId(i + 1))
    {
      std::cout << "Id list not properly sorted!" << std::endl;
      retVal = 1;
      break;
    }
  }
  std::cout << "Id list consistency check finished\n" << std::endl;

  //---------------------------------------------------------------------------
  // Sort id list (descending)
  std::cout << "Building id list (descending order)----------" << std::endl;
  ids->SetNumberOfIds(ARRAY_SIZE);
  for (i = 0; i < ARRAY_SIZE; i++)
  {
    ids->SetId(i, static_cast<vtkIdType>(vtkMath::Random(0, ARRAY_SIZE * 4)));
  }

  std::cout << "Sorting ids" << std::endl;
  timer->StartTimer();
  vtkSortDataArray::Sort(ids, 1);
  timer->StopTimer();

  std::cout << "Time to sort ids: " << timer->GetElapsedTime() << " sec" << std::endl;

  for (i = 0; i < ARRAY_SIZE - 1; i++)
  {
    if (ids->GetId(i) < ids->GetId(i + 1))
    {
      std::cout << "Id list not properly sorted!" << std::endl;
      retVal = 1;
      break;
    }
  }
  std::cout << "Id list consistency check finished\n" << std::endl;

  //---------------------------------------------------------------------------
  // Sort key/value pairs
  std::cout << "Building key/value arrays----------\n" << std::endl;
  vtkIntArray* values = vtkIntArray::New();
  values->SetNumberOfComponents(2);
  values->SetNumberOfTuples(ARRAY_SIZE);
  for (i = 0; i < ARRAY_SIZE; i++)
  {
    keys->SetComponent(i, 0, static_cast<int>(vtkMath::Random(0, ARRAY_SIZE * 4)));
    values->SetComponent(i, 0, i);
    values->SetComponent(i, 1, static_cast<int>(vtkMath::Random(0, ARRAY_SIZE * 4)));
  }
  vtkIntArray* saveKeys = vtkIntArray::New();
  saveKeys->DeepCopy(keys);
  vtkIntArray* saveValues = vtkIntArray::New();
  saveValues->DeepCopy(values);

  std::cout << "Sorting arrays" << std::endl;
  timer->StartTimer();
  vtkSortDataArray::Sort(keys, values);
  timer->StopTimer();

  std::cout << "Time to sort array: " << timer->GetElapsedTime() << " sec" << std::endl;

  for (i = 0; i < ARRAY_SIZE - 1; i++)
  {
    int lookup = static_cast<int>(values->GetComponent(i, 0));
    if (keys->GetComponent(i, 0) > keys->GetComponent(i + 1, 0))
    {
      std::cout << "Array not properly sorted!" << std::endl;
      retVal = 1;
      break;
    }
    if (keys->GetComponent(i, 0) != saveKeys->GetComponent(lookup, 0))
    {
      std::cout << "Values array not consistent with keys array!" << std::endl;
      retVal = 1;
      break;
    }
    if (values->GetComponent(i, 1) != saveValues->GetComponent(lookup, 1))
    {
      std::cout << "Values array not consistent with keys array!" << std::endl;
      retVal = 1;
      break;
    }
  }
  std::cout << "Array consistency check finished\n" << std::endl;

  std::cout << "Sorting sorted arrays" << std::endl;
  timer->StartTimer();
  vtkSortDataArray::Sort(keys, values);
  timer->StopTimer();

  std::cout << "Time to sort array: " << timer->GetElapsedTime() << " sec" << std::endl;

  for (i = 0; i < ARRAY_SIZE - 1; i++)
  {
    int lookup = static_cast<int>(values->GetComponent(i, 0));
    if (keys->GetComponent(i, 0) > keys->GetComponent(i + 1, 0))
    {
      std::cout << "Array not properly sorted!" << std::endl;
      retVal = 1;
      break;
    }
    if (keys->GetComponent(i, 0) != saveKeys->GetComponent(lookup, 0))
    {
      std::cout << "Values array not consistent with keys array!" << std::endl;
      retVal = 1;
      break;
    }
    if (values->GetComponent(i, 1) != saveValues->GetComponent(lookup, 1))
    {
      std::cout << "Values array not consistent with keys array!" << std::endl;
      retVal = 1;
      break;
    }
  }
  std::cout << "Array consistency check finished\n" << std::endl;

  //---------------------------------------------------------------------------
  // Sort data array on component value pairs
  std::cout << "Building data array----------\n" << std::endl;
  vtkFloatArray* fvalues = vtkFloatArray::New();
  fvalues->SetNumberOfComponents(3);
  fvalues->SetNumberOfTuples(ARRAY_SIZE);
  for (i = 0; i < ARRAY_SIZE; i++)
  {
    fvalues->SetComponent(i, 0, i);
    fvalues->SetComponent(i, 1, static_cast<float>(vtkMath::Random(0, ARRAY_SIZE * 4)));
    fvalues->SetComponent(i, 2, i);
  }
  vtkFloatArray* saveFValues = vtkFloatArray::New();
  saveFValues->DeepCopy(fvalues);

  std::cout << "Sorting data array with component #1" << std::endl;
  timer->StartTimer();
  vtkSortDataArray::SortArrayByComponent(fvalues, 1);
  timer->StopTimer();

  std::cout << "Time to sort data array: " << timer->GetElapsedTime() << " sec" << std::endl;

  for (i = 0; i < ARRAY_SIZE - 1; i++)
  {
    if (fvalues->GetComponent(i, 1) > fvalues->GetComponent(i + 1, 1))
    {
      std::cout << "Data array sorted incorrectly!" << std::endl;
      retVal = 1;
      break;
    }
    if (fvalues->GetComponent(i, 0) != fvalues->GetComponent(i, 2))
    {
      std::cout << "Data array tuples inconsistent!" << std::endl;
      retVal = 1;
      break;
    }
  }
  std::cout << "Data array consistency check finished\n" << std::endl;

  //---------------------------------------------------------------------------
  // Sort string array
  std::ostringstream ostr;
  ostr.imbue(std::locale::classic());
  std::cout << "Building string array----------\n" << std::endl;
  vtkStringArray* sarray = vtkStringArray::New();
  sarray->SetNumberOfTuples(ARRAY_SIZE);
  for (i = 0; i < ARRAY_SIZE; ++i)
  {
    ostr.str(""); // clear it out
    ostr << static_cast<int>(vtkMath::Random(0, ARRAY_SIZE * 4));
    sarray->SetValue(i, ostr.str());
  }

  std::cout << "Sorting string array" << std::endl;
  timer->StartTimer();
  vtkSortDataArray::Sort(sarray, 1);
  timer->StopTimer();
  std::cout << "Time to sort strings: " << timer->GetElapsedTime() << " sec" << std::endl;

  std::string s1, s2;
  for (i = 0; i < ARRAY_SIZE - 1; ++i)
  {
    s1 = sarray->GetValue(i);
    s2 = sarray->GetValue(i + 1);
    if (s1 < s2)
    {
      std::cout << "String array sorted incorrectly!" << std::endl;
      retVal = 1;
      break;
    }
  }
  std::cout << "String array consistency check finished\n" << std::endl;

  timer->Delete();
  keys->Delete();
  ids->Delete();
  values->Delete();
  fvalues->Delete();
  saveKeys->Delete();
  saveValues->Delete();
  saveFValues->Delete();
  sarray->Delete();

  return retVal;
}
