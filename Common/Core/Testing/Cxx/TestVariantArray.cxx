/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVariantArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkIntArray.h"
#include "vtkStringArray.h"
#include "vtkDoubleArray.h"
#include "vtkVariantArray.h"
#include "vtkArrayIterator.h"
#include "vtkArrayIteratorTemplate.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkSmartPointer.h"

#include <time.h>
#include <vector>
using std::vector;

void PrintArrays(vector<double> vec, vtkVariantArray* arr)
{
  cerr << endl;
  cerr << "index, vector, vtkVariantArray" << endl;
  cerr << "------------------------------" << endl;
  for (vtkIdType i = 0; i < arr->GetNumberOfValues(); i++)
    {
    cerr << i << ", " << vec[i] << ", " << arr->GetValue(i).ToDouble() << endl;
    }
  cerr << endl;
}

int TestLookup()
{
  vtkSmartPointer<vtkVariantArray> array =
    vtkSmartPointer<vtkVariantArray>::New();

  vtkSmartPointer<vtkIdList> idList =
    vtkSmartPointer<vtkIdList>::New();

  array->SetNumberOfValues(4);
  array->SetValue(0, "a");
  array->SetValue(1, "a");
  array->SetValue(2, "a");
  array->SetValue(3, "b");

  array->LookupValue("a", idList);
  if (idList->GetNumberOfIds() != 3)
    {
    cerr << "Expected 3 a's, found " << idList->GetNumberOfIds()
         << " of them\n";
    return 1;
    }

  if (idList->GetId(0) != 0 || idList->GetId(1) != 1 || idList->GetId(2) != 2)
    {
    cerr << "idList for a is wrong\n";
    return 1;
    }

  array->LookupValue("b", idList);
  if (idList->GetNumberOfIds() != 1)
    {
    cerr << "Expected 1 b, found " << idList->GetNumberOfIds()
         << " of them\n";
    return 1;
    }

  if (idList->GetId(0) != 3)
    {
    cerr << "idList for b is wrong\n";
    return 1;
    }

  array->SetValue(1, "b");

  array->LookupValue("a", idList);
  if (idList->GetNumberOfIds() != 2)
    {
    cerr << "Expected 2 a's, found " << idList->GetNumberOfIds()
         << " of them\n";
    return 1;
    }

  if (idList->GetId(0) != 0 || idList->GetId(1) != 2)
    {
    cerr << "idList for a is wrong\n";
    return 1;
    }

  array->LookupValue("b", idList);
  if (idList->GetNumberOfIds() != 2)
    {
    cerr << "Expected 2 b's, found " << idList->GetNumberOfIds()
         << " of them\n";
    return 1;
    }

  if (idList->GetId(0) != 1 || idList->GetId(1) != 3)
    {
    cerr << "idList for b is wrong\n";
    return 1;
    }

  return 0;
}

int TestVariantArray(int, char*[])
{
  cerr << "CTEST_FULL_OUTPUT" << endl;

  long seed = time(NULL);
  cerr << "Seed: " << seed << endl;
  vtkMath::RandomSeed(seed);

  int size = 20;
  double prob = 1.0 - 1.0 / size;

  vtkVariantArray* arr = vtkVariantArray::New();
  vector<double> vec;

  // Resizing
  // * int Allocate(vtkIdType sz);
  // * void Initialize();
  // * void SetNumberOfTuples(vtkIdType number);
  // * void Squeeze();
  // * int Resize(vtkIdType numTuples);
  // * void SetNumberOfValues(vtkIdType number);
  // * void SetVoidArray(void *arr, vtkIdType size, int save);
  // * void SetArray(vtkVariant* arr, vtkIdType size, int save);

  arr->Allocate(1000);
  if (arr->GetSize() != 1000 || arr->GetNumberOfTuples() != 0)
    {
    cerr << "size (" << arr->GetSize() << ") should be 1000, "
         << "tuples (" << arr->GetNumberOfTuples() << ") should be 0." << endl;
    exit(1);
    }

  arr->SetNumberOfValues(2000);
  if (arr->GetSize() != 2000 || arr->GetNumberOfTuples() != 2000)
    {
    cerr << "size (" << arr->GetSize() << ") should be 2000, "
         << "tuples (" << arr->GetNumberOfTuples() << ") should be 2000." << endl;
    exit(1);
    }

  arr->Initialize();
  if (arr->GetSize() != 0 || arr->GetNumberOfTuples() != 0)
    {
    cerr << "size (" << arr->GetSize() << ") should be 0, "
         << "tuples (" << arr->GetNumberOfTuples() << ") should be 0." << endl;
    exit(1);
    }

  arr->SetNumberOfComponents(3);

  arr->SetNumberOfTuples(1000);
  if (arr->GetSize() != 3000 || arr->GetNumberOfTuples() != 1000)
    {
    cerr << "size (" << arr->GetSize() << ") should be 3000, "
         << "tuples (" << arr->GetNumberOfTuples() << ") should be 1000." << endl;
    exit(1);
    }

  arr->SetNumberOfTuples(500);
  if (arr->GetSize() != 3000 || arr->GetNumberOfTuples() != 500)
    {
    cerr << "size (" << arr->GetSize() << ") should be 3000, "
         << "tuples (" << arr->GetNumberOfTuples() << ") should be 500." << endl;
    exit(1);
    }

  arr->Squeeze();
  if (arr->GetSize() != 1500 || arr->GetNumberOfTuples() != 500)
    {
    cerr << "size (" << arr->GetSize() << ") should be 1500, "
         << "tuples (" << arr->GetNumberOfTuples() << ") should be 500." << endl;
    exit(1);
    }

  arr->SetNumberOfTuples(1000);
  if (arr->GetSize() != 3000 || arr->GetNumberOfTuples() != 1000)
    {
    cerr << "size=" << arr->GetSize() << ", should be 3000, "
         << "tuples (" << arr->GetNumberOfTuples() << ", should be 1000." << endl;
    exit(1);
    }

  arr->Resize(500);
  if (arr->GetSize() != 1500 || arr->GetNumberOfTuples() != 500)
    {
    cerr << "size=" << arr->GetSize() << ", should be 1500, "
         << "tuples=" << arr->GetNumberOfTuples() << ", should be 500." << endl;
    exit(1);
    }

  vtkVariant* userArray = new vtkVariant[3000];
  arr->SetVoidArray(reinterpret_cast<void*>(userArray), 3000, 0);
  if (arr->GetSize() != 3000 || arr->GetNumberOfTuples() != 1000)
    {
    cerr << "size=" << arr->GetSize() << ", should be 3000, "
         << "tuples=" << arr->GetNumberOfTuples() << ", should be 1000." << endl;
    exit(1);
    }

  arr->SetNumberOfComponents(1);
  arr->Initialize();

  // Writing to the array
  // * void InsertValue(vtkIdType id, vtkVariant value);
  // * vtkIdType InsertNextValue(vtkVariant value);
  // * void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source);
  // * vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source);
  // * void SetValue(vtkIdType id, vtkVariant value);
  // * void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source);

  cerr << "Performing insert operations." << endl;
  vtkIdType id = 0;
  bool empty = true;
  while (empty || vtkMath::Random() < prob)
    {
    empty = false;
    if (vtkMath::Random() < 0.5)
      {
      arr->InsertValue(id, vtkVariant(id));
      }
    else
      {
      vtkIdType index = arr->InsertNextValue(vtkVariant(id));
      if (index != id)
        {
        cerr << "index=" << index << ", id=" << id << endl;
        exit(1);
        }
      }
    vec.push_back(id);
    id++;
    }

  vtkStringArray* stringArr = vtkStringArray::New();
  vtkIdType strId = id;
  empty = true;
  while (empty || vtkMath::Random() < prob)
    {
    empty = false;
    stringArr->InsertNextValue(vtkVariant(strId).ToString());
    strId++;
    }

  for (int i = 0; i < stringArr->GetNumberOfValues(); i++)
    {
    if (vtkMath::Random() < 0.5)
      {
      arr->InsertTuple(id, i, stringArr);
      }
    else
      {
      vtkIdType index = arr->InsertNextTuple(i, stringArr);
      if (index != id)
        {
        cerr << "index=" << index << ", id=" << id << endl;
        exit(1);
        }
      }
    vec.push_back(id);
    id++;
    }
  PrintArrays(vec, arr);

  cerr << "Performing set operations." << endl;
  while (vtkMath::Random() < prob)
    {
    int index = static_cast<int>(vtkMath::Random(0, arr->GetNumberOfValues()));
    if (vtkMath::Random() < 0.5)
      {
      arr->SetValue(index, vtkVariant(id));
      vec[index] = id;
      }
    else
      {
      int index2 = static_cast<int>(vtkMath::Random(0, stringArr->GetNumberOfValues()));
      arr->SetTuple(index, index2, stringArr);
      vec[index] = vtkVariant(stringArr->GetValue(index2)).ToDouble();
      }
    id++;
    }

  stringArr->Delete();

  PrintArrays(vec, arr);

  // Reading from the array
  // * unsigned long GetActualMemorySize();
  // * int IsNumeric();
  // * int GetDataType();
  // * int GetDataTypeSize();
  // * int GetElementComponentSize();
  // * vtkArrayIterator* NewIterator();
  // * vtkVariant & GetValue(vtkIdType id) const;
  // * vtkVariant* GetPointer(vtkIdType id);
  // * void *GetVoidPointer(vtkIdType id);
  // * vtkIdType GetNumberOfValues();
  // * void DeepCopy(vtkAbstractArray *da);
  //   void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
  //     vtkAbstractArray* source,  double* weights);
  //   void InterpolateTuple(vtkIdType i,
  //     vtkIdType id1, vtkAbstractArray* source1,
  //     vtkIdType id2, vtkAbstractArray* source2, double t);

  if (arr->IsNumeric())
    {
    cerr << "The variant array is reported to be numeric, but should not be." << endl;
    exit(1);
    }

  if (arr->GetDataType() != VTK_VARIANT)
    {
    cerr << "The type of the array should be VTK_VARIANT." << endl;
    exit(1);
    }

  if (arr->GetActualMemorySize() == 0
    || arr->GetDataTypeSize() == 0
    || arr->GetElementComponentSize() == 0)
    {
    cerr << "One of the size functions returned zero." << endl;
    exit(1);
    }

  if (arr->GetNumberOfValues() != static_cast<vtkIdType>(vec.size()))
    {
    cerr << "Sizes do not match ("
         << arr->GetNumberOfValues() << " != " << vec.size() << ")" << endl;
    exit(1);
    }

  cerr << "Checking by index." << endl;
  for (vtkIdType i = 0; i < arr->GetNumberOfValues(); i++)
    {
    double arrVal = arr->GetValue(i).ToDouble();
    if (arrVal != vec[i])
      {
      cerr << "values do not match (" << arrVal << " != " << vec[i] << ")" << endl;
      exit(1);
      }
    }

  cerr << "Check using an iterator." << endl;
  vtkArrayIteratorTemplate<vtkVariant>* iter
    = static_cast<vtkArrayIteratorTemplate<vtkVariant>*>(arr->NewIterator());
  for (vtkIdType i = 0; i < iter->GetNumberOfValues(); i++)
    {
    double arrVal = iter->GetValue(i).ToDouble();
    if (arrVal != vec[i])
      {
      cerr << "values do not match (" << arrVal << " != " << vec[i] << ")" << endl;
      exit(1);
      }
    }
  iter->Delete();

  cerr << "Check using array pointer." << endl;
  vtkVariant* pointer = reinterpret_cast<vtkVariant*>(arr->GetVoidPointer(0));
  for (vtkIdType i = 0; i < arr->GetNumberOfValues(); i++)
    {
    double arrVal = pointer[i].ToDouble();
    if (arrVal != vec[i])
      {
      cerr << "values do not match (" << arrVal << " != " << vec[i] << ")" << endl;
      exit(1);
      }
    }

  cerr << "Perform a deep copy and check it." << endl;
  vtkVariantArray* copy = vtkVariantArray::New();
  arr->DeepCopy(copy);
  for (vtkIdType i = 0; i < arr->GetNumberOfValues(); i++)
    {
    double arrVal = copy->GetValue(i).ToDouble();
    if (arrVal != vec[i])
      {
      cerr << "values do not match (" << arrVal << " != " << vec[i] << ")" << endl;
      exit(1);
      }
    }
  copy->Delete();

  arr->Delete();

  if (int result = TestLookup())
    {
    return result;
    }

  return 0;
}

