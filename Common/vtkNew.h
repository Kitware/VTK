/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNew.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNew - Allocate and hold a VTK object.
// .SECTION Description
// vtkNew<T> is a class template that on construction allocates and
// initializes an instance of its template argument using T::New().
// It assumes ownership of one reference during its lifetime, and
// frees it on destruction.
//
// Example usage:
//
//   vtkNew<vtkClass> a;
//   a->SomeMethod();
//
//   vtkSmartPointer<vtkBaseClass> b = vtkNew<vtkDerivedClass>();
//   b->SomeVirtualMethod();

#ifndef __vtkNew_h
#define __vtkNew_h

#include "vtkIOStream.h"

class vtkObjectBase;

template <class T>
class vtkNew
{
  void CheckObjectBase(vtkObjectBase*) {}
public:
  // Description:
  // Creates a new T on construction.
  vtkNew(): Object(T::New())
    {
    this->CheckObjectBase(this->Object);
    }

  // Description:
  // Deletes reference to instance of T on destruction.
  ~vtkNew()
    {
    if(T* object = this->Object)
      {
      this->Object = 0;
      object->Delete();
      }
    }

  // Description:
  // Enable pointer-like dereference syntax.
  // Returns pointer to contained object.
  T* operator->() const
    {
    return this->Object;
    }

  // Description:
  // Enable conversion to a raw pointer.
  // Returns pointer to contained object.
  operator T*() const
    {
    return this->Object;
    }
private:
  vtkNew(vtkNew<T> const&);         // Not implemented.
  void operator=(vtkNew<T> const&); // Not implemented.
  T* Object;
};

// Description:
// Streaming operator to print vtkNew objects like regular pointers.
template <class T>
inline ostream& operator << (ostream& os, const vtkNew<T>& p)
{
  return os << static_cast<T*>(p);
}

#endif
