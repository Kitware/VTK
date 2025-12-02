// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkArrayIterator.h"
#include "vtkArrayIteratorTemplate.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkVariantArray.h"

#include <time.h>
#include <vector>

#include <iostream>

using std::vector;

void PrintArrays(vector<double> vec, vtkVariantArray* arr)
{
  std::cerr << std::endl;
  std::cerr << "index, vector, vtkVariantArray" << std::endl;
  std::cerr << "------------------------------" << std::endl;
  for (vtkIdType i = 0; i < arr->GetNumberOfValues(); i++)
  {
    std::cerr << i << ", " << vec[i] << ", " << arr->GetValue(i).ToDouble() << std::endl;
  }
  std::cerr << std::endl;
}

int TestLookup()
{
  vtkSmartPointer<vtkVariantArray> array = vtkSmartPointer<vtkVariantArray>::New();

  vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();

  array->SetNumberOfValues(4);
  array->SetValue(0, "a");
  array->SetValue(1, "a");
  array->SetValue(2, "a");
  array->SetValue(3, "b");

  array->LookupValue("a", idList);
  if (idList->GetNumberOfIds() != 3)
  {
    std::cerr << "Expected 3 a's, found " << idList->GetNumberOfIds() << " of them\n";
    return 1;
  }

  if (idList->GetId(0) != 0 || idList->GetId(1) != 1 || idList->GetId(2) != 2)
  {
    std::cerr << "idList for a is wrong\n";
    return 1;
  }

  array->LookupValue("b", idList);
  if (idList->GetNumberOfIds() != 1)
  {
    std::cerr << "Expected 1 b, found " << idList->GetNumberOfIds() << " of them\n";
    return 1;
  }

  if (idList->GetId(0) != 3)
  {
    std::cerr << "idList for b is wrong\n";
    return 1;
  }

  array->SetValue(1, "b");

  array->LookupValue("a", idList);
  if (idList->GetNumberOfIds() != 2)
  {
    std::cerr << "Expected 2 a's, found " << idList->GetNumberOfIds() << " of them\n";
    return 1;
  }

  if (idList->GetId(0) != 0 || idList->GetId(1) != 2)
  {
    std::cerr << "idList for a is wrong\n";
    return 1;
  }

  array->LookupValue("b", idList);
  if (idList->GetNumberOfIds() != 2)
  {
    std::cerr << "Expected 2 b's, found " << idList->GetNumberOfIds() << " of them\n";
    return 1;
  }

  if (idList->GetId(0) != 1 || idList->GetId(1) != 3)
  {
    std::cerr << "idList for b is wrong\n";
    return 1;
  }

  return 0;
}

int TestVariantArray(int, char*[])
{
  std::cerr << "CTEST_FULL_OUTPUT" << std::endl;

  long seed = time(nullptr);
  std::cerr << "Seed: " << seed << std::endl;
  vtkMath::RandomSeed(seed);

  int size = 20;
  double prob = 1.0 - 1.0 / size;

  vtkVariantArray* arr = vtkVariantArray::New();

  // Resizing
  // * vtkTypeBool Allocate(vtkIdType sz);
  // * void Initialize();
  // * void SetNumberOfTuples(vtkIdType number);
  // * void Squeeze();
  // * vtkTypeBool Resize(vtkIdType numTuples);
  // * void SetNumberOfValues(vtkIdType number);
  // * void SetVoidArray(void *arr, vtkIdType size, int save);
  // * void SetArray(vtkVariant* arr, vtkIdType size, int save);

  arr->Allocate(1000);
  if (arr->GetSize() != 1000 || arr->GetNumberOfTuples() != 0)
  {
    std::cerr << "size (" << arr->GetSize() << ") should be 1000, "
              << "tuples (" << arr->GetNumberOfTuples() << ") should be 0." << std::endl;
    exit(1);
  }

  arr->SetNumberOfValues(2000);
  if (arr->GetSize() != 2000 || arr->GetNumberOfTuples() != 2000)
  {
    std::cerr << "size (" << arr->GetSize() << ") should be 2000, "
              << "tuples (" << arr->GetNumberOfTuples() << ") should be 2000." << std::endl;
    exit(1);
  }

  arr->Initialize();
  if (arr->GetSize() != 0 || arr->GetNumberOfTuples() != 0)
  {
    std::cerr << "size (" << arr->GetSize() << ") should be 0, "
              << "tuples (" << arr->GetNumberOfTuples() << ") should be 0." << std::endl;
    exit(1);
  }

  arr->SetNumberOfComponents(3);

  arr->SetNumberOfTuples(1000);
  if (arr->GetSize() != 3000 || arr->GetNumberOfTuples() != 1000)
  {
    std::cerr << "size (" << arr->GetSize() << ") should be 3000, "
              << "tuples (" << arr->GetNumberOfTuples() << ") should be 1000." << std::endl;
    exit(1);
  }

  arr->SetNumberOfTuples(500);
  if (arr->GetSize() != 3000 || arr->GetNumberOfTuples() != 500)
  {
    std::cerr << "size (" << arr->GetSize() << ") should be 3000, "
              << "tuples (" << arr->GetNumberOfTuples() << ") should be 500." << std::endl;
    exit(1);
  }

  arr->Squeeze();
  if (arr->GetSize() != 1500 || arr->GetNumberOfTuples() != 500)
  {
    std::cerr << "size (" << arr->GetSize() << ") should be 1500, "
              << "tuples (" << arr->GetNumberOfTuples() << ") should be 500." << std::endl;
    exit(1);
  }

  arr->SetNumberOfTuples(1000);
  if (arr->GetSize() != 3000 || arr->GetNumberOfTuples() != 1000)
  {
    std::cerr << "size=" << arr->GetSize() << ", should be 3000, "
              << "tuples (" << arr->GetNumberOfTuples() << ", should be 1000." << std::endl;
    exit(1);
  }

  arr->Resize(500);
  if (arr->GetSize() != 1500 || arr->GetNumberOfTuples() != 500)
  {
    std::cerr << "size=" << arr->GetSize() << ", should be 1500, "
              << "tuples=" << arr->GetNumberOfTuples() << ", should be 500." << std::endl;
    exit(1);
  }

  vtkVariant* userArray = new vtkVariant[3000];
  arr->SetVoidArray(reinterpret_cast<void*>(userArray), 3000, 0);
  if (arr->GetSize() != 3000 || arr->GetNumberOfTuples() != 1000)
  {
    std::cerr << "size=" << arr->GetSize() << ", should be 3000, "
              << "tuples=" << arr->GetNumberOfTuples() << ", should be 1000." << std::endl;
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

  std::cerr << "Performing insert operations." << std::endl;
  vtkIdType id = 0;
  bool empty = true;
  vector<double> vec;
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
        std::cerr << "index=" << index << ", id=" << id << std::endl;
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
        std::cerr << "index=" << index << ", id=" << id << std::endl;
        exit(1);
      }
    }
    vec.push_back(id);
    id++;
  }
  PrintArrays(vec, arr);

  std::cerr << "Performing set operations." << std::endl;
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
    std::cerr << "The variant array is reported to be numeric, but should not be." << std::endl;
    exit(1);
  }

  if (arr->GetDataType() != VTK_VARIANT)
  {
    std::cerr << "The type of the array should be VTK_VARIANT." << std::endl;
    exit(1);
  }

  if (arr->GetActualMemorySize() == 0 || arr->GetDataTypeSize() == 0 ||
    arr->GetElementComponentSize() == 0)
  {
    std::cerr << "One of the size functions returned zero." << std::endl;
    exit(1);
  }

  if (arr->GetNumberOfValues() != static_cast<vtkIdType>(vec.size()))
  {
    std::cerr << "Sizes do not match (" << arr->GetNumberOfValues() << " != " << vec.size() << ")"
              << std::endl;
    exit(1);
  }

  std::cerr << "Checking by index." << std::endl;
  for (vtkIdType i = 0; i < arr->GetNumberOfValues(); i++)
  {
    double arrVal = arr->GetValue(i).ToDouble();
    if (arrVal != vec[i])
    {
      std::cerr << "values do not match (" << arrVal << " != " << vec[i] << ")" << std::endl;
      exit(1);
    }
  }

  std::cerr << "Check using an iterator." << std::endl;
  vtkArrayIteratorTemplate<vtkVariant>* iter =
    static_cast<vtkArrayIteratorTemplate<vtkVariant>*>(arr->NewIterator());
  for (vtkIdType i = 0; i < iter->GetNumberOfValues(); i++)
  {
    double arrVal = iter->GetValue(i).ToDouble();
    if (arrVal != vec[i])
    {
      std::cerr << "values do not match (" << arrVal << " != " << vec[i] << ")" << std::endl;
      exit(1);
    }
  }
  iter->Delete();

  std::cerr << "Check using array pointer." << std::endl;
  vtkVariant* pointer = reinterpret_cast<vtkVariant*>(arr->GetVoidPointer(0));
  for (vtkIdType i = 0; i < arr->GetNumberOfValues(); i++)
  {
    double arrVal = pointer[i].ToDouble();
    if (arrVal != vec[i])
    {
      std::cerr << "values do not match (" << arrVal << " != " << vec[i] << ")" << std::endl;
      exit(1);
    }
  }

  std::cerr << "Perform a deep copy and check it." << std::endl;
  vtkVariantArray* copy = vtkVariantArray::New();
  arr->DeepCopy(copy);
  for (vtkIdType i = 0; i < arr->GetNumberOfValues(); i++)
  {
    double arrVal = copy->GetValue(i).ToDouble();
    if (arrVal != vec[i])
    {
      std::cerr << "values do not match (" << arrVal << " != " << vec[i] << ")" << std::endl;
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
