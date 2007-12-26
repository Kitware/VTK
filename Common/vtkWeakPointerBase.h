/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWeakPointerBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWeakPointerBase - Non-templated superclass for vtkWeakPointer.
// .SECTION Description
// vtkWeakPointerBase holds a pointer to a vtkObject or subclass
// instance, but it never affects the reference count of the vtkObject. However,
// when the vtkObject referred to is destroyed, the pointer gets initialized to
// NULL, thus avoid dangling references.

#ifndef __vtkWeakPointerBase_h
#define __vtkWeakPointerBase_h

#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkWeakPointerBase
{
public:
  // Description:
  // Initialize smart pointer to NULL.
  vtkWeakPointerBase();

  // Description:
  // Initialize smart pointer to given object.
  vtkWeakPointerBase(vtkObject* r);

  // Description:
  // Initialize weak pointer .
  vtkWeakPointerBase(const vtkWeakPointerBase& r);

  // Description:
  // Destroy smart pointer.
  ~vtkWeakPointerBase();

  // Description:
  // Assign object to reference.  This removes any reference to an old
  // object.
  vtkWeakPointerBase& operator=(vtkObject* r);
  vtkWeakPointerBase& operator=(const vtkWeakPointerBase& r);

  // Description:
  // Get the contained pointer.
  vtkObject* GetPointer() const
    {
    // Inline implementation so smart pointer comparisons can be fully
    // inlined.
    return this->Object;
    }

protected:

  // Initialize smart pointer to given object, but do not increment
  // reference count.  The destructor will still decrement the count.
  // This effectively makes it an auto-ptr.
  class NoReference {};
  vtkWeakPointerBase(vtkObject* r, const NoReference&);

  // Pointer to the actual object.
  vtkObject* Object;

private:
  // Internal utility methods.
  void RemoveObserver();
  void AddObserver();

  class vtkObserver;
  friend class vtkObserver;
  vtkObserver* Observer;
};

//----------------------------------------------------------------------------
// Need to use vtkstd_bool type because std: :less requires bool return
// type from operators.  This example should not be used to justify
// using bool elsewhere in VTK.

#define VTK_WEAK_POINTER_BASE_DEFINE_OPERATOR(op) \
  inline vtkstd_bool \
  operator op (const vtkWeakPointerBase& l, const vtkWeakPointerBase& r) \
    { \
    return (static_cast<void*>(l.GetPointer()) op \
            static_cast<void*>(r.GetPointer())); \
    } \
  inline vtkstd_bool \
  operator op (vtkObject* l, const vtkWeakPointerBase& r) \
    { \
    return (static_cast<void*>(l) op static_cast<void*>(r.GetPointer())); \
    } \
  inline vtkstd_bool \
  operator op (const vtkWeakPointerBase& l, vtkObject* r) \
    { \
    return (static_cast<void*>(l.GetPointer()) op static_cast<void*>(r)); \
    }
// Description:
// Compare smart pointer values.
VTK_WEAK_POINTER_BASE_DEFINE_OPERATOR(==)
VTK_WEAK_POINTER_BASE_DEFINE_OPERATOR(!=)
VTK_WEAK_POINTER_BASE_DEFINE_OPERATOR(<)
VTK_WEAK_POINTER_BASE_DEFINE_OPERATOR(<=)
VTK_WEAK_POINTER_BASE_DEFINE_OPERATOR(>)
VTK_WEAK_POINTER_BASE_DEFINE_OPERATOR(>=)

#undef VTK_WEAK_POINTER_BASE_DEFINE_OPERATOR

// Description:
// Streaming operator to print smart pointer like regular pointers.
VTK_COMMON_EXPORT ostream& operator << (ostream& os,
                                        const vtkWeakPointerBase& p);

#endif
