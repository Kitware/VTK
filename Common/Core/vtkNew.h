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
/**
 * @class   vtkNew
 * @brief   Allocate and hold a VTK object.
 *
 * vtkNew is a class template that on construction allocates and
 * initializes an instance of its template argument using T::New().
 * It assumes ownership of one reference during its lifetime, and calls
 * T->Delete() on destruction.
 *
 * Automatic casting to raw pointer is available for convenience, but
 * users of this method should ensure that they do not
 * return this pointer if the vtkNew will go out of scope without
 * incrementing its reference count.
 *
 * vtkNew is a drop in replacement for vtkSmartPointer, for example,
 *
 * \code
 * vtkNew<vtkRenderer> ren;
 * vtkNew<vtkRenderWindow> renWin;
 * renWin->AddRenderer(ren);
 * vtkNew<vtkRenderWindowInteractor> iren;
 * iren->SetRenderWindow(renWin);
 * \endcode
 *
 *
 * @sa
 * vtkSmartPointer vtkWeakPointer
*/

#ifndef vtkNew_h
#define vtkNew_h

#include "vtkIOStream.h"

class vtkObjectBase;

template <class T>
class vtkNew
{
  /**
   * Compile time checking that the class is derived from vtkObjectBase.
   */
  void CheckObjectBase(vtkObjectBase*) {}
public:
  /**
   * Create a new T on construction.
   */
  vtkNew() : Object(T::New())
  {
    this->CheckObjectBase(this->Object);
  }

  //@{
  /**
   * Deletes reference to instance of T on destruction.
   */
  ~vtkNew()
  {
    T* obj = this->Object;
    if (obj)
    {
      this->Object = nullptr;
      obj->Delete();
    }
  }
  //@}

  /**
   * Enable pointer-like dereference syntax. Returns a pointer to the contained
   * object.
   */
  T* operator->() const
  {
    return this->Object;
  }

  //@{
  /**
   * Get a raw pointer to the contained object. When using this function be
   * careful that the reference count does not drop to 0 when using the pointer
   * returned. This will happen when the vtkNew object goes out of
   * scope for example.
   */
  T* GetPointer() const
  {
    return this->Object;
  }
  T* Get() const
  {
    return this->Object;
  }
  operator T* () const
  {
    return static_cast<T*>(this->Object);
  }
  //@}
  /**
   * Dereference the pointer and return a reference to the contained object.
   * When using this function be careful that the reference count does not
   * drop to 0 when using the pointer returned.
   * This will happen when the vtkNew object goes out of scope for example.
   */
  T& operator*() const
  {
    return *static_cast<T*>(this->Object);
  }

private:
  vtkNew(vtkNew<T> const&) = delete;
  void operator=(vtkNew<T> const&) = delete;
  T* Object;
};

#endif
// VTK-HeaderTest-Exclude: vtkNew.h
