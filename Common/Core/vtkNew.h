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
// vtkNew is a class template that on construction allocates and
// initializes an instance of its template argument using T::New().
// It assumes ownership of one reference during its lifetime, and calls
// T->Delete() on destruction.
//
// Automatic casting is intentionally unavailable, calling GetPointer() will
// return a raw pointer. Users of this method should ensure that they do not
// return this pointer if the vtkNew will go out of scope without
// incrementing its reference count using vtkSmartPointer or similar.
//
// \code
// vtkNew<vtkClass> a;
// a->SomeMethod();
//
// vtkSmartPointer<vtkClass> b = a.GetPointer();
// b->SomeOtherMethod();
// \endcode
//
// It should be noted that vtkNew is not a drop in replacement for
// vtkSmartPointer as it is not implicitly cast to a pointer in functions
// requiring a pointer. The GetPointer() method must be used, for example,
//
// \code
// vtkNew<vtkRenderer> ren;
// vtkNew<vtkRenderWindow> renWin;
// renWin->AddRenderer(ren.GetPointer());
// vtkNew<vtkRenderWindowInteractor> iren;
// iren->SetRenderWindow(renWin.GetPointer());
// \endcode
//
// .SECTION See Also
// vtkSmartPointer vtkWeakPointer

#ifndef __vtkNew_h
#define __vtkNew_h

#include "vtkIOStream.h"

class vtkObjectBase;

template <class T>
class vtkNew
{
  // Description:
  // Compile time checking that the class is derived from vtkObjectBase.
  void CheckObjectBase(vtkObjectBase*) {}
public:
  // Description:
  // Create a new T on construction.
  vtkNew() : Object(T::New())
    {
    this->CheckObjectBase(this->Object);
    }

  // Description:
  // Deletes reference to instance of T on destruction.
  ~vtkNew()
    {
    T* obj = this->Object;
    if (obj)
      {
      this->Object = 0;
      obj->Delete();
      }
    }

  // Description:
  // Enable pointer-like dereference syntax. Returns a pointer to the contained
  // object.
  T* operator->() const
    {
    return this->Object;
    }

  // Description:
  // Get a raw pointer to the contained object. When using this function be
  // careful that the reference count does not drop to 0 when using the pointer
  // returned. This will happen when the vtkNew object goes out of
  // scope for example.
  T* GetPointer() const
    {
    return this->Object;
    }
  T* Get() const
    {
    return this->Object;
    }

private:
  vtkNew(vtkNew<T> const&); // Not implemented.
  void operator=(vtkNew<T> const&);   // Not implemented.
  T* Object;
};

// Description:
// Streaming operator to print vtkNew like regular pointers.
template <class T>
inline ostream& operator << (ostream& os, const vtkNew<T>& p)
{
  return os << p.GetPointer();
}

#endif
// VTK-HeaderTest-Exclude: vtkNew.h
