/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSmartPointer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
  // Get the contained pointer.
  operator T* () const
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

  // Description:
  // Create an instance of a VTK object.
  static vtkSmartPointer<T> New()
    {
    return vtkSmartPointer<T>(T::New(), NoReference());
    }

  // Description:
  // Create a new instance of the given VTK object.
  static vtkSmartPointer<T> NewInstance(T* t)
    {
    return vtkSmartPointer<T>(t->NewInstance(), NoReference());
    }

  // Work-around for HP overload resolution bug.
#if defined(__HP_aCC)
# define VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(op) \
  vtkstd_bool operator op (const vtkSmartPointer<T>& r)   \
    {                                                     \
    return ::operator op (*this, r);                      \
    }                                                     \
  vtkstd_bool operator op (T* r)                          \
    {                                                     \
    return ::operator op (*this, r);                      \
    }
#else
# define VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(op)
#endif
  VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(==)
  VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(!=)
  VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(<)
  VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(<=)
  VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(>)
  VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(>=)
#undef VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND

protected:
  vtkSmartPointer(T* r, const NoReference& n): vtkSmartPointerBase(r, n) {}
};

#define VTK_SMART_POINTER_DEFINE_OPERATOR(op) \
  template <class T> \
  inline vtkstd_bool \
  operator op (const vtkSmartPointer<T>& l, const vtkSmartPointer<T>& r) \
    { \
    return (l.GetPointer() op r.GetPointer()); \
    } \
  template <class T> \
  inline vtkstd_bool operator op (T* l, const vtkSmartPointer<T>& r) \
    { \
    return (l op r.GetPointer()); \
    } \
  template <class T> \
  inline vtkstd_bool operator op (const vtkSmartPointer<T>& l, T* r) \
    { \
    return (l.GetPointer() op r); \
    }
// Description:
// Compare smart pointer values.
VTK_SMART_POINTER_DEFINE_OPERATOR(==)
VTK_SMART_POINTER_DEFINE_OPERATOR(!=)
VTK_SMART_POINTER_DEFINE_OPERATOR(<)
VTK_SMART_POINTER_DEFINE_OPERATOR(<=)
VTK_SMART_POINTER_DEFINE_OPERATOR(>)
VTK_SMART_POINTER_DEFINE_OPERATOR(>=)

#undef VTK_SMART_POINTER_DEFINE_OPERATOR

// Description:
// Streaming operator to print smart pointer like regular pointers.
template <class T>
inline ostream& operator << (ostream& os, const vtkSmartPointer<T>& p)
{
  return os << static_cast<const vtkSmartPointerBase&>(p);
}

#endif
