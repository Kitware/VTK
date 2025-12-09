// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCollection.h"
#include "vtkCollectionRange.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"

#include <algorithm>

#include <iostream>

bool TestRegister();
bool TestRemoveItem(int index, bool removeIndex);
bool TestGeneral();

int TestCollection(int, char*[])
{
  bool res = true;
  res = TestRegister() && res;
  res = TestRemoveItem(0, false) && res;
  res = TestRemoveItem(1, false) && res;
  res = TestRemoveItem(5, false) && res;
  res = TestRemoveItem(8, false) && res;
  res = TestRemoveItem(9, false) && res;
  res = TestRemoveItem(0, true) && res;
  res = TestRemoveItem(1, true) && res;
  res = TestRemoveItem(5, true) && res;
  res = TestRemoveItem(8, true) && res;
  res = TestRemoveItem(9, true) && res;
  res = TestGeneral() && res;
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}

static bool IsEqualRange(
  vtkCollection* collection, const std::vector<vtkSmartPointer<vtkIntArray>>& v)
{
  const auto range = vtk::Range(collection);
  if (range.size() != static_cast<int>(v.size()))
  {
    std::cerr << "Range size invalid.\n";
    return false;
  }

  // Test C++11 for-range interop
  auto vecIter = v.begin();
  for (auto item : range)
  {
    if (item != vecIter->GetPointer())
    {
      std::cerr << "Range iterator returned unexpected value.\n";
      return false;
    }
    ++vecIter;
  }

  return true;
}

static bool IsEqual(vtkCollection* collection, const std::vector<vtkSmartPointer<vtkIntArray>>& v)
{
  if (collection->GetNumberOfItems() != static_cast<int>(v.size()))
  {
    return false;
  }
  vtkIntArray* dataArray = nullptr;
  vtkCollectionSimpleIterator it;
  int i = 0;
  for (collection->InitTraversal(it);
       (dataArray = vtkIntArray::SafeDownCast(collection->GetNextItemAsObject(it))); ++i)
  {
    if (v[i] != dataArray)
    {
      return false;
    }
  }
  return IsEqualRange(collection, v); // test range iterators, too.
}

bool TestRegister()
{
  vtkNew<vtkCollection> collection;
  vtkIntArray* object = vtkIntArray::New();
  collection->AddItem(object);
  object->Delete();
  if (object->GetReferenceCount() != 1)
  {
    std::cout << object->GetReferenceCount() << std::endl;
    return false;
  }
  object->Register(nullptr);
  collection->RemoveItem(object);
  if (object->GetReferenceCount() != 1)
  {
    std::cout << object->GetReferenceCount() << std::endl;
    return false;
  }
  object->UnRegister(nullptr);
  return true;
}

bool TestRemoveItem(int index, bool removeIndex)
{
  vtkNew<vtkCollection> collection;
  std::vector<vtkSmartPointer<vtkIntArray>> objects;
  constexpr int expectedCount = 10;
  for (int i = 0; i < expectedCount; ++i)
  {
    vtkNew<vtkIntArray> object;
    collection->AddItem(object);
    objects.emplace_back(object.GetPointer());
  }

  // These should do nothing.
  collection->RemoveItem(nullptr);
  collection->RemoveItem(-1);
  collection->RemoveItem(expectedCount);
  if (collection->GetNumberOfItems() != expectedCount)
  {
    std::cerr << "Nop operations did something.\n";
    return false;
  }
  if (collection->IsItemPresent(nullptr) != 0)
  {
    std::cerr << "IsItemPresent found null in collection.\n";
    return false;
  }
  if (collection->IndexOfFirstOccurrence(nullptr) != -1)
  {
    std::cerr << "IndexOfFirstOccurrence found null in collection.\n";
    return false;
  }

  if (removeIndex)
  {
    collection->RemoveItem(index);
  }
  else
  {
    vtkObject* objectToRemove = objects[index];
    collection->RemoveItem(objectToRemove);
  }
  objects.erase(objects.begin() + index);
  if (!IsEqual(collection, objects))
  {
    std::cout << "TestRemoveItem failed:" << std::endl;
    collection->Print(std::cout);
    return false;
  }
  return true;
}

bool TestGeneral()
{
  vtkNew<vtkIntArray> a1;
  a1->InsertNextValue(1);

  vtkNew<vtkIntArray> a2;
  a2->InsertNextValue(2);

  vtkNew<vtkIntArray> a3;
  a3->InsertNextValue(3);

  vtkNew<vtkIntArray> a4;
  a3->InsertNextValue(4);

  vtkMTimeType time1, time2;

  // Should start empty.
  vtkNew<vtkCollection> collection;
  if (collection->GetNumberOfItems() != 0)
  {
    return false;
  }

  // Removing all from nothing does nothing. modified time also should not change.
  time1 = collection->GetMTime();
  collection->RemoveAllItems();
  time2 = collection->GetMTime();
  if (collection->GetNumberOfItems() != 0 || time1 != time2)
  {
    return false;
  }

  // InsertItem documented to do nothing if collection is empty, regardless of parameters. modified
  // time also should not change.
  time1 = collection->GetMTime();
  collection->InsertItem(0, a1);
  collection->InsertItem(-1, a1);
  collection->InsertItem(1, a1);
  collection->InsertItem(0, nullptr);
  collection->InsertItem(-1, nullptr);
  collection->InsertItem(1, nullptr);
  time2 = collection->GetMTime();
  if (collection->GetNumberOfItems() != 0 || time1 != time2)
  {
    return false;
  }

  // Add one item.
  time1 = collection->GetMTime();
  collection->AddItem(a1);
  time2 = collection->GetMTime();
  if (collection->GetNumberOfItems() != 1 || time2 <= time1)
  {
    return false;
  }
  // Now we have: a1

  // idx=1 is out of range, so should do nothing. modified time also should not change.
  time1 = collection->GetMTime();
  collection->InsertItem(1, a2);
  time2 = collection->GetMTime();
  if (collection->GetNumberOfItems() != 1 || time2 != time1)
  {
    return false;
  }

  // negative index documented to add to the beginning.
  collection->InsertItem(-1, a2);
  if (collection->GetNumberOfItems() != 2)
  {
    return false;
  }
  collection->InsertItem(INT_MIN, a3);
  if (collection->GetNumberOfItems() != 3)
  {
    return false;
  }
  if ((collection->GetItemAsObject(-1) != nullptr) || (collection->GetItemAsObject(0) != a3) ||
    (collection->GetItemAsObject(1) != a2) || (collection->GetItemAsObject(2) != a1) ||
    (collection->GetItemAsObject(3) != nullptr))
  {
    return false;
  }
  // Now we have: a3,a2,a1

  // out-of-range ReplaceItem documented to do nothing
  time1 = collection->GetMTime();
  collection->ReplaceItem(-1, a2);
  collection->ReplaceItem(3, a2);
  time2 = collection->GetMTime();
  if ((collection->GetItemAsObject(-1) != nullptr) || (collection->GetItemAsObject(0) != a3) ||
    (collection->GetItemAsObject(1) != a2) || (collection->GetItemAsObject(2) != a1) ||
    (collection->GetItemAsObject(3) != nullptr) || time2 != time1)
  {
    return false;
  }

  // Actually reorder with ReplaceItem.
  time1 = collection->GetMTime();
  collection->ReplaceItem(0, a1);
  time2 = collection->GetMTime();
  collection->ReplaceItem(1, a3);
  collection->ReplaceItem(2, a2);
  if ((collection->GetItemAsObject(-1) != nullptr) || (collection->GetItemAsObject(0) != a1) ||
    (collection->GetItemAsObject(1) != a3) || (collection->GetItemAsObject(2) != a2) ||
    (collection->GetItemAsObject(3) != nullptr) || time2 <= time1)
  {
    return false;
  }
  // Now we have: a1,a3,a2

  //
  time1 = collection->GetMTime();
  int idx = collection->IsItemPresent(nullptr);
  if (idx != 0)
  {
    return false;
  }
  idx = collection->IsItemPresent(a1);
  if (idx != 1)
  {
    return false;
  }
  idx = collection->IsItemPresent(a3);
  if (idx != 2)
  {
    return false;
  }
  idx = collection->IsItemPresent(a2);
  if (idx != 3)
  {
    return false;
  }
  idx = collection->IsItemPresent(a4);
  if (idx != 0)
  {
    return false;
  }
  time2 = collection->GetMTime();
  if (time2 != time1)
  {
    return false;
  }

  // Test 0-based indexes.
  time1 = collection->GetMTime();
  idx = collection->IndexOfFirstOccurrence(nullptr);
  if (idx != -1)
  {
    return false;
  }
  idx = collection->IndexOfFirstOccurrence(a1);
  if (idx != 0)
  {
    return false;
  }
  idx = collection->IndexOfFirstOccurrence(a3);
  if (idx != 1)
  {
    return false;
  }
  idx = collection->IndexOfFirstOccurrence(a2);
  if (idx != 2)
  {
    return false;
  }
  idx = collection->IndexOfFirstOccurrence(a4);
  if (idx != -1)
  {
    return false;
  }
  time2 = collection->GetMTime();
  if (time2 != time1)
  {
    return false;
  }

  // Add a second a1.
  collection->AddItem(a1);
  if (collection->GetNumberOfItems() != 4)
  {
    return false;
  }
  // Now we have: a1,a3,a2,a1

  // Remove the first a1.
  collection->RemoveItem(a1);
  if ((collection->GetItemAsObject(0) != a3) || (collection->GetItemAsObject(1) != a2) ||
    (collection->GetItemAsObject(2) != a1))
  {
    return false;
  }
  // Now we have: a3,a2,a1

  // "Remove" non-present item.
  collection->RemoveItem(a4);
  if (collection->GetNumberOfItems() != 3)
  {
    return false;
  }

  // --- Simple traversal.
  time1 = collection->GetMTime();
  collection->InitTraversal();
  if (collection->GetNextItemAsObject() != a3)
  {
    return false;
  }
  if (collection->GetNextItemAsObject() != a2)
  {
    return false;
  }
  if (collection->GetNextItemAsObject() != a1)
  {
    return false;
  }
  if (collection->GetNextItemAsObject() != nullptr)
  {
    return false;
  }
  if (collection->GetNextItemAsObject() != nullptr)
  {
    return false;
  }
  time2 = collection->GetMTime();
  if (time2 != time1)
  {
    return false;
  }

  // --- Simple traversal with cookie.
  time1 = collection->GetMTime();
  vtkCollectionSimpleIterator cookie = nullptr;
  collection->InitTraversal(cookie);
  if (collection->GetNextItemAsObject(cookie) != a3)
  {
    return false;
  }
  if (collection->GetNextItemAsObject(cookie) != a2)
  {
    return false;
  }
  if (collection->GetNextItemAsObject(cookie) != a1)
  {
    return false;
  }
  if (collection->GetNextItemAsObject(cookie) != nullptr)
  {
    return false;
  }
  if (collection->GetNextItemAsObject(cookie) != nullptr)
  {
    return false;
  }
  time2 = collection->GetMTime();
  if (time2 != time1)
  {
    return false;
  }

  // --- Simple traversal with InitTraversal() again halfway through.
  time1 = collection->GetMTime();
  collection->InitTraversal();
  if (collection->GetNextItemAsObject() != a3)
  {
    return false;
  }
  if (collection->GetNextItemAsObject() != a2)
  {
    return false;
  }
  collection->InitTraversal();
  if (collection->GetNextItemAsObject() != a3)
  {
    return false;
  }
  if (collection->GetNextItemAsObject() != a2)
  {
    return false;
  }
  time2 = collection->GetMTime();
  if (time2 != time1)
  {
    return false;
  }

  // --- RemoveItem during traversal (case 1, at current).
  // Now we have: a3,a2,a1
  collection->InitTraversal();
  if (collection->GetNextItemAsObject() != a3)
  {
    return false;
  }
  collection->RemoveItem(1); // Removes a2, which would have been next.
  if (collection->GetNextItemAsObject() != a1)
  {
    return false;
  }
  if (collection->GetNextItemAsObject() != nullptr)
  {
    return false;
  }
  // Now we have: a3,a1

  // --- RemoveItem during traversal (case 2, at front).
  // Now we have: a3,a1
  collection->InitTraversal();
  collection->RemoveItem(0); // Removes a3
  if (collection->GetNextItemAsObject() != a1)
  {
    return false;
  }
  if (collection->GetNextItemAsObject() != nullptr)
  {
    return false;
  }
  // Now we have: a1

  // --- RemoveItem during traversal (case 3, only item).
  // Now we have: a1
  collection->InitTraversal();
  collection->RemoveItem(0); // Removes a1
  if (collection->GetNextItemAsObject() != nullptr)
  {
    return false;
  }
  // Now we have: nothing

  // --- RemoveItem during traversal (case 4, item before current).
  collection->AddItem(a1);
  collection->AddItem(a2);
  collection->AddItem(a3);
  collection->AddItem(a4);
  collection->InitTraversal();
  if (collection->GetNextItemAsObject() != a1)
  {
    return false;
  }
  if (collection->GetNextItemAsObject() != a2)
  {
    return false;
  }
  collection->RemoveItem(0); // Removes a1
  if (collection->GetNextItemAsObject() != a3)
  {
    return false;
  }
  if (collection->GetNextItemAsObject() != a4)
  {
    return false;
  }
  // Now we have: a2,a3,a4

  // --- RemoveItem during traversal (case 5, item after current).
  collection->InitTraversal();
  if (collection->GetNextItemAsObject() != a2)
  {
    return false;
  }
  collection->RemoveItem(2); // Removes a4
  if (collection->GetNextItemAsObject() != a3)
  {
    return false;
  }
  if (collection->GetNextItemAsObject() != nullptr)
  {
    return false;
  }
  // Now we have: a2,a3

  // --- RemoveAllItems during traversal.
  collection->AddItem(a4);
  collection->InitTraversal();
  if (collection->GetNextItemAsObject() != a2)
  {
    return false;
  }
  collection->RemoveAllItems(); // Removes a2,a3,a4
  if (collection->GetNextItemAsObject() != nullptr)
  {
    return false;
  }
  // Now we have: nothing

  // --- Replace item with itself.
  // TODO: current implementation changes MTime even for this no-op. Not sure if that's a feature or
  // missed optimization.
  collection->AddItem(a1);
  collection->AddItem(a2);
  time1 = collection->GetMTime();
  collection->ReplaceItem(0, a1);
  time2 = collection->GetMTime();
  if ((collection->GetItemAsObject(0) != a1) || (collection->GetItemAsObject(1) != a2) ||
    (time1 == time2))
  {
    return false;
  }
  // Now we have: a1,a2

  // --- Replace during traversal.
  collection->AddItem(a3);
  collection->InitTraversal();
  if (collection->GetNextItemAsObject() != a1)
  {
    return false;
  }
  collection->ReplaceItem(1, a4);
  if (collection->GetNextItemAsObject() != a4)
  {
    return false;
  }
  if (collection->GetNextItemAsObject() != a3)
  {
    return false;
  }
  if (collection->GetNextItemAsObject() != nullptr)
  {
    return false;
  }
  // Now we have: a1,a4,a3

  return true;
}
