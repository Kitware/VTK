/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSmartPointerBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSmartPointerBase
 * @brief   Non-templated superclass for vtkSmartPointer.
 *
 * vtkSmartPointerBase holds a pointer to a vtkObjectBase or subclass
 * instance and performs one Register/UnRegister pair.  This is useful
 * for storing VTK objects in STL containers.  This class is not
 * intended to be used directly.  Instead, use the vtkSmartPointer
 * class template to automatically perform proper cast operations.
*/

#ifndef vtkSmartPointerBase_h
#define vtkSmartPointerBase_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObjectBase.h"

class VTKCOMMONCORE_EXPORT vtkSmartPointerBase
{
public:
  /**
   * Initialize smart pointer to NULL.
   */
  vtkSmartPointerBase();

  /**
   * Initialize smart pointer to given object.
   */
  vtkSmartPointerBase(vtkObjectBase* r);

  /**
   * Initialize smart pointer with a new reference to the same object
   * referenced by given smart pointer.
   */
  vtkSmartPointerBase(const vtkSmartPointerBase& r);

  /**
   * Destroy smart pointer and remove the reference to its object.
   */
  ~vtkSmartPointerBase();

  //@{
  /**
   * Assign object to reference.  This removes any reference to an old
   * object.
   */
  vtkSmartPointerBase& operator=(vtkObjectBase* r);
  vtkSmartPointerBase& operator=(const vtkSmartPointerBase& r);
  //@}

  /**
   * Get the contained pointer.
   */
  vtkObjectBase* GetPointer() const
  {
    // Inline implementation so smart pointer comparisons can be fully
    // inlined.
    return this->Object;
  }

  /**
   * Report the reference held by the smart pointer to a collector.
   */
  void Report(vtkGarbageCollector* collector, const char* desc);

protected:

  // Initialize smart pointer to given object, but do not increment
  // reference count.  The destructor will still decrement the count.
  // This effectively makes it an auto-ptr.
  class NoReference {};
  vtkSmartPointerBase(vtkObjectBase* r, const NoReference&);

  // Pointer to the actual object.
  vtkObjectBase* Object;

private:
  // Internal utility methods.
  void Swap(vtkSmartPointerBase& r);
  void Register();
};

//----------------------------------------------------------------------------
#define VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(op) \
  inline bool \
  operator op (const vtkSmartPointerBase& l, const vtkSmartPointerBase& r) \
  { \
    return (static_cast<void*>(l.GetPointer()) op \
            static_cast<void*>(r.GetPointer())); \
  } \
  inline bool \
  operator op (vtkObjectBase* l, const vtkSmartPointerBase& r) \
  { \
    return (static_cast<void*>(l) op static_cast<void*>(r.GetPointer())); \
  } \
  inline bool \
  operator op (const vtkSmartPointerBase& l, vtkObjectBase* r) \
  { \
    return (static_cast<void*>(l.GetPointer()) op static_cast<void*>(r)); \
  }
/**
 * Compare smart pointer values.
 */
VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(==)
VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(!=)
VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(<)
VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(<=)
VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(>)
VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(>=)

#undef VTK_SMART_POINTER_BASE_DEFINE_OPERATOR

/**
 * Streaming operator to print smart pointer like regular pointers.
 */
VTKCOMMONCORE_EXPORT ostream& operator << (ostream& os,
                                        const vtkSmartPointerBase& p);

#endif
// VTK-HeaderTest-Exclude: vtkSmartPointerBase.h
