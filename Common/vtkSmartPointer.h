/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSmartPointer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSmartPointer - Hold a reference to a vtkObjectBase instance.
// .SECTION Description
// vtkSmartPointer is a class template that provides automatic casting
// for objects held by the vtkSmartPointerBase superclass.

#ifndef __vtkSmartPointer_h
#define __vtkSmartPointer_h

#include "vtkSmartPointerBase.h"

template <class T>
class vtkSmartPointer: public vtkSmartPointerBase
{
public:
  // Description:
  // Initialize smart pointer to NULL.
  vtkSmartPointer() {}
  
  // Description:
  // Initialize smart pointer to given object.
  vtkSmartPointer(T* r): vtkSmartPointerBase(r) {}
  
  // Description:
  // Initialize smart pointer with a new reference to the same object
  // referenced by given smart pointer.
  vtkSmartPointer(const vtkSmartPointerBase& r): vtkSmartPointerBase(r) {}
  
  // Description:
  // Assign object to reference.  This removes any reference to an old
  // object.
  vtkSmartPointer& operator=(T* r)
    {
    this->vtkSmartPointerBase::operator=(r);
    return *this;
    }
  
  // Description:
  // Assign object to reference.  This removes any reference to an old
  // object.
  vtkSmartPointer& operator=(const vtkSmartPointerBase& r)
    {
    this->vtkSmartPointerBase::operator=(r);
    return *this;
    }  
  
  // Description:
  // Get the contained pointer.
  T* GetPointer() const
    {
    return static_cast<T*>(this->Object);
    }
  
  // Description:
  // Dereference the pointer and return a reference to the contained
  // object.
  T& operator*() const
    {
    return *static_cast<T*>(this->Object);
    }
  
  // Description:
  // Provides normal pointer target member access using operator ->.
  T* operator->() const
    {
    return static_cast<T*>(this->Object);
    }
};

#endif
