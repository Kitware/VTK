// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkStandaloneSession
 * @brief A standalone session for managing VTK objects in a WebAssembly environment.
 *
 * This class provides an interface for creating, destroying, and interacting with VTK objects
 * in a WebAssembly context. It allows setting and retrieving properties, invoking methods,
 * and managing event observers for VTK objects.
 *
 * This class is designed to work with Emscripten through JavaScript bindings.
 *
 * @note This class is part of the WebAssembly module internals and is not exported for general use.
 *
 * @sa vtkObjectManager, vtkRemoteSession
 */

#ifndef vtkStandaloneSession_h
#define vtkStandaloneSession_h

#include "vtkSession.h"           // for vtkSession
#include "vtkWebAssemblyModule.h" // for no export macro

#include <emscripten/val.h> // for emscripten::val

VTK_ABI_NAMESPACE_BEGIN

class VTKWEBASSEMBLY_NO_EXPORT vtkStandaloneSession
{
public:
  vtkStandaloneSession();
  ~vtkStandaloneSession();

  /**
   * Create an object of type `className`.
   */
  vtkObjectHandle Create(const std::string& className);

  /**
   * Destroy a VTKObject
   */
  void Destroy(vtkObjectHandle object);

  /**
   * Set properties of a VTKObject
   */
  void Set(vtkObjectHandle object, emscripten::val properties);

  /**
   * Get all properties of a VTKObject
   */
  emscripten::val Get(vtkObjectHandle object);

  /**
   * Invoke a function `methodName` on `object` with `args` and return the result in a
   * VTKJson.
   */
  emscripten::val Invoke(
    vtkObjectHandle object, const std::string& methodName, emscripten::val args);

  /**
   * Add an observer to a VTKObject for a specific event.
   */
  unsigned long Observe(
    vtkObjectHandle object, const std::string& eventName, emscripten::val jsFunction);

  /**
   * Remove an observer from a VTKObject.
   */
  bool UnObserve(vtkObjectHandle object, unsigned long tag);

private:
  vtkSession Session;
};

VTK_ABI_NAMESPACE_END
#endif
