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
// vtkWeakPointerBase holds a pointer to a vtkObjectBase or subclass
// instance, but it never affects the reference count of the vtkObjectBase. However,
// when the vtkObjectBase referred to is destroyed, the pointer gets initialized to
// NULL, thus avoid dangling references.

#ifndef __vtkWeakPointerBase_h
#define __vtkWeakPointerBase_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObjectBase.h"

class vtkObjectBaseToWeakPointerBaseFriendship;

class VTKCOMMONCORE_EXPORT vtkWeakPointerBase
{
public:
  // Description:
  // Initialize smart pointer to NULL.
  vtkWeakPointerBase() : Object(0) {}

  // Description:
  // Initialize smart pointer to given object.
  vtkWeakPointerBase(vtkObjectBase* r);

  // Description:
  // Initialize weak pointer .
  vtkWeakPointerBase(const vtkWeakPointerBase& r);

  // Description:
  // Destroy smart pointer.
  ~vtkWeakPointerBase();

  // Description:
  // Assign object to reference.  This removes any reference to an old
  // object.
  vtkWeakPointerBase& operator=(vtkObjectBase* r);
  vtkWeakPointerBase& operator=(const vtkWeakPointerBase& r);

  // Description:
  // Get the contained pointer.
  vtkObjectBase* GetPointer() const
    {
    // Inline implementation so smart pointer comparisons can be fully
    // inlined.
    return this->Object;
    }

private:
  friend class vtkObjectBaseToWeakPointerBaseFriendship;

protected:

  // Initialize weak pointer to given object.
  class NoReference {};
  vtkWeakPointerBase(vtkObjectBase* r, const NoReference&);

  // Pointer to the actual object.
  vtkObjectBase* Object;
};

//----------------------------------------------------------------------------
#define VTK_WEAK_POINTER_BASE_DEFINE_OPERATOR(op) \
  inline bool \
  operator op (const vtkWeakPointerBase& l, const vtkWeakPointerBase& r) \
    { \
    return (static_cast<void*>(l.GetPointer()) op \
            static_cast<void*>(r.GetPointer())); \
    } \
  inline bool \
  operator op (vtkObjectBase* l, const vtkWeakPointerBase& r) \
    { \
    return (static_cast<void*>(l) op static_cast<void*>(r.GetPointer())); \
    } \
  inline bool \
  operator op (const vtkWeakPointerBase& l, vtkObjectBase* r) \
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
VTKCOMMONCORE_EXPORT ostream& operator << (ostream& os,
                                        const vtkWeakPointerBase& p);

#endif
// VTK-HeaderTest-Exclude: vtkWeakPointerBase.h
