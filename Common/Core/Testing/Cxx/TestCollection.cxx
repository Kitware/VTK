/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCollection.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"

bool TestRegister();
bool TestRemoveItem(int index, bool removeIndex);

int TestCollection(int,char *[])
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
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}

bool IsEqual(vtkCollection* collection, const std::vector<vtkSmartPointer<vtkIntArray> >& v)
{
  if (collection->GetNumberOfItems() != static_cast<int>(v.size()))
    {
    return false;
    }
  vtkIntArray* dataArray = 0;
  vtkCollectionSimpleIterator it;
  int i = 0;
  for (collection->InitTraversal(it);
       (dataArray = vtkIntArray::SafeDownCast(collection->GetNextItemAsObject(it))) ; ++i)
    {
    if (v[i] != dataArray)
      {
      return false;
      }
    }
  return true;
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
  object->Register(0);
  collection->RemoveItem(object);
  if (object->GetReferenceCount() != 1)
    {
    std::cout << object->GetReferenceCount() << std::endl;
    return false;
    }
  object->UnRegister(0);
  return true;
}

bool TestRemoveItem(int index, bool removeIndex)
{
  vtkNew<vtkCollection> collection;
  std::vector<vtkSmartPointer<vtkIntArray> > objects;
  for (int i = 0; i < 10; ++i)
    {
    vtkNew<vtkIntArray> object;
    collection->AddItem(object.GetPointer());
    objects.push_back(object.GetPointer());
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
  if (!IsEqual(collection.GetPointer(), objects))
    {
    std::cout << "TestRemoveItem failed:" << std::endl;
    collection->Print(std::cout);
    return false;
    }
  return true;
}
