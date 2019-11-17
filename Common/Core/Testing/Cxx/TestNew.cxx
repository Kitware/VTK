/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSmartPointer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkNew.
// .SECTION Description
// Tests instantiations of the vtkNew class template.

#include "vtkDebugLeaks.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include "vtkTestNewVar.h"

int TestNew(int, char*[])
{
  bool error = false;
  // This one should be cleaned up when the main function ends.
  vtkNew<vtkIntArray> a;
  if (a->GetReferenceCount() != 1)
  {
    error = true;
    cerr << "Error, reference count should be 1, was " << a->GetReferenceCount() << endl;
  }
  cout << "vtkNew streaming " << a << endl;

  vtkWeakPointer<vtkFloatArray> wf;
  // Test scoping, and deletion.
  if (wf == nullptr)
  {
    vtkNew<vtkFloatArray> f;
    wf = f;
  }
  if (wf != nullptr)
  {
    error = true;
    cerr << "Error, vtkNew failed to delete the object it contained." << endl;
  }
  // Test implicit conversion vtkNew::operator T* () const
  if (wf == nullptr)
  {
    vtkNew<vtkFloatArray> f;
    wf = f;
  }
  if (wf != nullptr)
  {
    error = true;
    cerr << "Error, vtkNew failed to delete the object it contained (implicit cast to raw pointer)."
         << endl;
  }

  // Now test interaction with the smart pointer.
  vtkSmartPointer<vtkIntArray> si;
  if (si == nullptr)
  {
    vtkNew<vtkIntArray> i;
    si = i;
  }
  if (si->GetReferenceCount() != 1)
  {
    error = true;
    cerr << "Error, vtkNew failed to delete the object it contained, "
         << "or the smart pointer failed to increment it. Reference count: "
         << si->GetReferenceCount() << endl;
  }

  // Test raw object reference
  vtkObject& p = *si;
  if (p.GetReferenceCount() != 1)
  {
    error = true;
    cerr << "Error, vtkNew failed to keep the object it contained, "
         << "or setting a raw reference incremented it. Reference count: " << p.GetReferenceCount()
         << endl;
  }

  vtkNew<vtkTestNewVar> newVarObj;
  if (newVarObj->GetPointsRefCount() != 1)
  {
    error = true;
    cerr << "The member pointer failed to set the correct reference count: "
         << newVarObj->GetPointsRefCount() << endl;
  }

  vtkSmartPointer<vtkObject> points = newVarObj->GetPoints();
  if (points->GetReferenceCount() != 2)
  {
    error = true;
    cerr << "Error, vtkNew failed to keep the object it contained, "
         << "or the smart pointer failed to increment it. Reference count: "
         << points->GetReferenceCount() << endl;
  }
  vtkSmartPointer<vtkObject> points2 = newVarObj->GetPoints2();
  if (points2->GetReferenceCount() != 3)
  {
    error = true;
    cerr << "Error, vtkNew failed to keep the object it contained, "
         << "or the smart pointer failed to increment it. Reference count: "
         << points->GetReferenceCount() << endl;
  }

  vtkNew<vtkIntArray> intarray;
  vtkIntArray* intarrayp = intarray.GetPointer();
  if (intarrayp != intarray || intarray != intarrayp)
  {
    error = true;
    cerr << "Error, comparison of vtkNew object to it's raw pointer fails\n";
  }

  {
    vtkNew<vtkIntArray> testArray1;
    vtkNew<vtkIntArray> testArray2(std::move(testArray1));
    if (testArray1 || !testArray2)
    {
      std::cerr << "Error, move construction of vtkNew failed.\n";
      error = true;
    }
    vtkNew<vtkDataArray> testArray3(std::move(testArray2));
    if (testArray2 || !testArray3)
    {
      std::cerr << "Error, move construction of vtkNew failed.\n";
      error = true;
    }
  }

  return error ? 1 : 0;
}
