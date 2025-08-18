// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkCocoaAutoreleasePool_h
#define vtkCocoaAutoreleasePool_h

#include "vtkRenderingUIModule.h" // For export macro
#include "vtkSystemIncludes.h"

/**
 *  @class   vtkCocoaAutoreleasePool
 *  @brief   RAII class to create an NSAutoreleasePool
 */
VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGUI_EXPORT vtkCocoaAutoreleasePool
{
public:
  vtkCocoaAutoreleasePool();
  ~vtkCocoaAutoreleasePool();

  /**
   * Release the pool to reclaim the memory.
   * When using this class from Python, call this method at the end of the
   * current scope, since object lifetimes in Python are not sufficiently
   * deterministic for basic RAII.  In C++, this method is unnecessary and
   * you can rely on the destructor to release the pool.
   */
  void Release();

private:
  vtkCocoaAutoreleasePool(const vtkCocoaAutoreleasePool&) = delete;
  vtkCocoaAutoreleasePool& operator=(const vtkCocoaAutoreleasePool&) = delete;

  void* Pool; // Pointer to NSAutoreleasePool
};

VTK_ABI_NAMESPACE_END

#endif // vtkCocoaAutoreleasePool_h
