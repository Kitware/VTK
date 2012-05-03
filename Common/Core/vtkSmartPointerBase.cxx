/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSmartPointerBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointerBase.h"

#include "vtkGarbageCollector.h"

//----------------------------------------------------------------------------
vtkSmartPointerBase::vtkSmartPointerBase():
  Object(0)
{
  // Add a reference to the object.
  this->Register();
}

//----------------------------------------------------------------------------
vtkSmartPointerBase::vtkSmartPointerBase(vtkObjectBase* r):
  Object(r)
{
  // Add a reference to the object.
  this->Register();
}

//----------------------------------------------------------------------------
vtkSmartPointerBase::vtkSmartPointerBase(vtkObjectBase* r, const NoReference&):
  Object(r)
{
  // Do not add a reference to the object because we received the
  // NoReference argument.
}

//----------------------------------------------------------------------------
vtkSmartPointerBase::vtkSmartPointerBase(const vtkSmartPointerBase& r):
  Object(r.Object)
{
  // Add a reference to the object.
  this->Register();
}

//----------------------------------------------------------------------------
vtkSmartPointerBase::~vtkSmartPointerBase()
{
  // The main pointer must be set to NULL before calling UnRegister,
  // so use a local variable to save the pointer.  This is because the
  // garbage collection reference graph traversal may make it back to
  // this smart pointer, and we do not want to include this reference.
  vtkObjectBase* object = this->Object;
  if(object)
    {
    this->Object = 0;
    object->UnRegister(0);
    }
}

//----------------------------------------------------------------------------
vtkSmartPointerBase&
vtkSmartPointerBase::operator=(vtkObjectBase* r)
{
  // This is an exception-safe assignment idiom that also gives the
  // correct order of register/unregister calls to all objects
  // involved.  A temporary is constructed that references the new
  // object.  Then the main pointer and temporary are swapped and the
  // temporary's destructor unreferences the old object.
  vtkSmartPointerBase(r).Swap(*this);
  return *this;
}

//----------------------------------------------------------------------------
vtkSmartPointerBase&
vtkSmartPointerBase::operator=(const vtkSmartPointerBase& r)
{
  // This is an exception-safe assignment idiom that also gives the
  // correct order of register/unregister calls to all objects
  // involved.  A temporary is constructed that references the new
  // object.  Then the main pointer and temporary are swapped and the
  // temporary's destructor unreferences the old object.
  vtkSmartPointerBase(r).Swap(*this);
  return *this;
}

//----------------------------------------------------------------------------
void vtkSmartPointerBase::Report(vtkGarbageCollector* collector,
                                 const char* desc)
{
  vtkGarbageCollectorReport(collector, this->Object, desc);
}

//----------------------------------------------------------------------------
void vtkSmartPointerBase::Swap(vtkSmartPointerBase& r)
{
  // Just swap the pointers.  This is used internally by the
  // assignment operator.
  vtkObjectBase* temp = r.Object;
  r.Object = this->Object;
  this->Object = temp;
}

//----------------------------------------------------------------------------
void vtkSmartPointerBase::Register()
{
  // Add a reference only if the object is not NULL.
  if(this->Object)
    {
    this->Object->Register(0);
    }
}

//----------------------------------------------------------------------------
ostream& operator << (ostream& os, const vtkSmartPointerBase& p)
{
  // Just print the pointer value into the stream.
  return os << static_cast<void*>(p.GetPointer());
}
