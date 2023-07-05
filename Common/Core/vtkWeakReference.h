// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkWeakReference
 * @brief   Utility class to hold a weak reference to a vtkObject.
 *
 * Simple Set(...)/Get(...) interface. Used in numpy support to provide a
 * reference to a vtkObject without preventing it from being collected.
 */

#ifndef vtkWeakReference_h
#define vtkWeakReference_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkWeakPointer.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkWeakReference : public vtkObject
{
public:
  vtkTypeMacro(vtkWeakReference, vtkObject);
  static vtkWeakReference* New();
  vtkWeakReference();
  ~vtkWeakReference() override;

  /**
   * Set the vtkObject to maintain a weak reference to.
   */
  void Set(vtkObject* object);

  /**
   * Get the vtkObject pointer or nullptr if the object has been collected.
   */
  vtkObject* Get();

private:
  vtkWeakPointer<vtkObject> Object;
};

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkWeakReference.h
