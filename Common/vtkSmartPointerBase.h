/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSmartPointerBase.h
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
// .NAME vtkSmartPointerBase - Non-templated superclass for vtkSmartPointer.
// .SECTION Description
// vtkSmartPointerBase holds a pointer to a vtkObjectBase or subclass
// instance and performs one Register/UnRegister pair.  This is useful
// for storing VTK objects in STL containers.  This class is not
// intended to be used directly.  Instead, use the vtkSmartPointer
// class template to automatically perform proper cast operations.

#ifndef __vtkSmartPointerBase_h
#define __vtkSmartPointerBase_h

#include "vtkObjectBase.h"

class VTK_COMMON_EXPORT vtkSmartPointerBase
{
public:
  // Description:
  // Initialize smart pointer to NULL.
  vtkSmartPointerBase();
  
  // Description:
  // Initialize smart pointer to given object.
  vtkSmartPointerBase(vtkObjectBase* r);
  
  // Description:
  // Initialize smart pointer with a new reference to the same object
  // referenced by given smart pointer.
  vtkSmartPointerBase(const vtkSmartPointerBase& r);
  
  // Description:
  // Destroy smart pointer and remove the reference to its object.
  ~vtkSmartPointerBase();
  
  // Description:
  // Assign object to reference.  This removes any reference to an old
  // object.
  vtkSmartPointerBase& operator=(vtkObjectBase* r);
  vtkSmartPointerBase& operator=(const vtkSmartPointerBase& r);
  
  // Description:
  // Get the contained pointer.
  vtkObjectBase* GetPointer() const;
protected:
  
  // Internal utility methods.
  void Swap(vtkSmartPointerBase& r);
  void Register();
  void UnRegister();
  
  // Pointer to the actual object.
  vtkObjectBase* Object;
};

// Description:
// Compare smart pointer values.
#define VTK_SMART_POINTER_BASE_DECLARE_OPERATOR(op) \
  VTK_COMMON_EXPORT int operator op (const vtkSmartPointerBase& l, \
                                     const vtkSmartPointerBase& r); \
  VTK_COMMON_EXPORT int operator op (vtkObjectBase* l, \
                                     const vtkSmartPointerBase& r); \
  VTK_COMMON_EXPORT int operator op (const vtkSmartPointerBase& l, \
                                     vtkObjectBase* r)
VTK_SMART_POINTER_BASE_DECLARE_OPERATOR(==);  
VTK_SMART_POINTER_BASE_DECLARE_OPERATOR(!=);
VTK_SMART_POINTER_BASE_DECLARE_OPERATOR(<);
VTK_SMART_POINTER_BASE_DECLARE_OPERATOR(<=);
VTK_SMART_POINTER_BASE_DECLARE_OPERATOR(>);
VTK_SMART_POINTER_BASE_DECLARE_OPERATOR(>=);
#undef VTK_SMART_POINTER_BASE_DECLARE_OPERATOR

// Description:
// Streaming operator to print smart pointer like regular pointers.
VTK_COMMON_EXPORT ostream& operator << (ostream& os,
                                        const vtkSmartPointerBase& p);

#endif
