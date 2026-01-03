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
 * @sa vtkRemoteSession
 */

#ifndef vtkStandaloneSession_h
#define vtkStandaloneSession_h

#include "vtkType.h"                     // for vtkTypeUInt32
#include "vtkWebAssemblySessionModule.h" // for no export macro

#include <emscripten/val.h> // for emscripten::val

VTK_ABI_NAMESPACE_BEGIN

typedef struct vtkSessionImpl* vtkSession;
class VTKWEBASSEMBLYSESSION_EXPORT vtkStandaloneSession
{
public:
  vtkStandaloneSession();
  ~vtkStandaloneSession();

  /**
   * Create an object of type `className`.
   */
  vtkTypeUInt32 Create(const std::string& className);

  /**
   * Destroy a VTKObject
   */
  bool Destroy(vtkTypeUInt32 object);

  /**
   * Set properties of a VTKObject
   */
  bool Set(vtkTypeUInt32 object, emscripten::val properties);

  /**
   * Get all properties of a VTKObject
   */
  emscripten::val Get(vtkTypeUInt32 object);

  /**
   * Invoke a function `methodName` on `object` with `args` and return the result in a
   * VTKJson.
   */
  emscripten::val Invoke(vtkTypeUInt32 object, const std::string& methodName, emscripten::val args);

  /**
   * Add an observer to a VTKObject for a specific event.
   */
  unsigned long Observe(
    vtkTypeUInt32 object, const std::string& eventName, emscripten::val jsFunction);

  /**
   * Remove an observer from a VTKObject.
   */
  bool UnObserve(vtkTypeUInt32 object, unsigned long tag);

  /**
   * @brief Retrieves the total memory usage of all blobs.
   * @return The total memory usage in bytes.
   */
  std::size_t GetTotalBlobMemoryUsage();

  /**
   * @brief Retrieves the total memory usage of all VTK data objects.
   * @return The total memory usage in bytes.
   */
  std::size_t GetTotalVTKDataObjectMemoryUsage();

  /**
   * @brief Prints information about the scene manager.
   */
  void PrintSceneManagerInformation();

  /**
   * @brief Sets the verbosity level for the deserializer log.
   * @param verbosityLevel The desired verbosity level.
   */
  void SetDeserializerLogVerbosity(const std::string& verbosityLevel);

  /**
   * @brief Sets the verbosity level for the invoker log.
   * @param verbosityLevel The desired verbosity level.
   */
  void SetInvokerLogVerbosity(const std::string& verbosityLevel);

  /**
   * @brief Sets the verbosity level for the object manager log.
   * @param verbosityLevel The desired verbosity level.
   */
  void SetObjectManagerLogVerbosity(const std::string& verbosityLevel);

  /**
   * @brief Sets the verbosity level for the serializer log.
   * @param verbosityLevel The desired verbosity level.
   */
  void SetSerializerLogVerbosity(const std::string& verbosityLevel);

  vtkSession Session;
};

VTK_ABI_NAMESPACE_END
#endif
