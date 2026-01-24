// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkRemoteSession
 * @brief Provides a remote session interface wrapped around vtk.h for managing VTK objects and
 * states in a WebAssembly environment.
 *
 * The `vtkRemoteSession` class allows interaction with VTK objects and their states in a
 * WebAssembly context. It provides methods for registering, unregistering, and retrieving states
 * and blobs, as well as invoking methods on VTK objects. Additionally, it supports resizing
 * windows, rendering scene, resetting camera, starting/stopping interactor event loops, and
 * retrieving object dependencies. The class also includes utilities for pruning unused resources
 * and importing/exporting states and blobs.
 *
 * Key features include:
 * - State and blob management.
 * - Rendering and camera control.
 * - Event observation and un-observation.
 * - Dependency management and memory usage tracking.
 * - Logging verbosity configuration for various components.
 *
 * This class is designed to work with Emscripten through JavaScript bindings.
 *
 * @note This class is part of the WebAssembly module internals and is not exported for general use.
 *
 * @sa vtkStandaloneSession
 */

#ifndef vtkRemoteSession_h
#define vtkRemoteSession_h

#include "vtkType.h"                     // for vtkTypeUInt32
#include "vtkWebAssemblySessionModule.h" // for no export macro

#include <emscripten/val.h> // for emscripten::val

VTK_ABI_NAMESPACE_BEGIN

typedef struct vtkSessionImpl* vtkSession;
class VTKWEBASSEMBLYSESSION_EXPORT vtkRemoteSession
{
public:
  /**
   * Constructor for vtkRemoteSession initializes the session.
   */
  vtkRemoteSession();

  /**
   * Destructor for vtkRemoteSession cleans up the session.
   */
  ~vtkRemoteSession();

  /**
   * @brief Registers a state with the session.
   * @param state A JavaScript object representing the state to register.
   * @return True if the state was successfully registered, false otherwise.
   */
  bool RegisterState(emscripten::val state);

  /**
   * @brief Unregisters a state associated with a VTK object handle.
   * @param object The handle of the VTK object whose state is to be unregistered.
   * @return True if the state was successfully unregistered, false otherwise.
   */
  bool UnRegisterState(vtkTypeUInt32 object);

  /**
   * Set properties of a VTKObject
   * @param object The handle of the VTK object.
   * @param properties A JavaScript object representing the properties to set.
   * @return True if the properties were successfully set, false otherwise.
   */
  bool Set(vtkTypeUInt32 object, emscripten::val properties);

  /**
   * Get all properties of a VTKObject
   * @param object The handle of the VTK object.
   * @return A JavaScript object representing the properties.
   */
  emscripten::val Get(vtkTypeUInt32 object);

  /**
   * @brief Skips a property during serialization or deserialization.
   * @param className The name of the class containing the property.
   * @param propertyName The name of the property to skip.
   */
  void SkipProperty(const std::string& className, const std::string& propertyName);

  /**
   * @brief Unskips a previously skipped property.
   * @param className The name of the class containing the property.
   * @param propertyName The name of the property to unskip.
   */
  void UnSkipProperty(const std::string& className, const std::string& propertyName);

  /**
   * @brief Registers a binary blob with the session.
   * @param hash A unique hash identifying the blob.
   * @param jsArray A JavaScript array containing the blob data.
   * @return True if the blob was successfully registered, false otherwise.
   */
  bool RegisterBlob(const std::string& hash, emscripten::val jsArray);

  /**
   * @brief Unregisters a binary blob from the session.
   * @param hash The unique hash identifying the blob.
   * @return True if the blob was successfully unregistered, false otherwise.
   */
  bool UnRegisterBlob(const std::string& hash);

  /**
   * @brief Retrieves a binary blob by its hash.
   * @param hash The unique hash identifying the blob.
   * @return A JavaScript array containing the blob data.
   */
  emscripten::val GetBlob(const std::string& hash);

  /**
   * @brief Invokes a method on a VTK object.
   * @param object The handle of the VTK object.
   * @param methodName The name of the method to invoke.
   * @param args A JavaScript object containing the method arguments.
   * @return A JavaScript object representing the result of the method invocation.
   */
  emscripten::val Invoke(vtkTypeUInt32 object, const std::string& methodName, emscripten::val args);

  /**
   * @brief Retrieves all dependencies of a VTK object.
   * @param object The handle of the VTK object.
   * @return A JavaScript array representing the dependencies.
   */
  emscripten::val GetAllDependencies(vtkTypeUInt32 object);

  /**
   * @brief Updates a VTK object from a given state.
   * @param state A JavaScript object representing the state.
   * @return True if the object was successfully updated, false otherwise.
   */
  bool UpdateObjectFromState(emscripten::val state);

  /**
   * @brief Updates the state from a given VTK object.
   * @param object The handle of the VTK object.
   */
  void UpdateStateFromObject(vtkTypeUInt32 object);

  /**
   * @brief Sets the size of a VTK object.
   * @param object The handle of the VTK object.
   * @param width The desired width.
   * @param height The desired height.
   * @return True if the size was successfully set, false otherwise.
   */
  bool SetSize(vtkTypeUInt32 object, int width, int height);

  /**
   * @brief Renders a VTK object.
   * @param object The handle of the VTK object.
   * @return True if the rendering was successful, false otherwise.
   */
  bool Render(vtkTypeUInt32 object);

  /**
   * @brief Resets the camera for a VTK object.
   * @param object The handle of the VTK object.
   * @return True if the camera was successfully reset, false otherwise.
   */
  bool ResetCamera(vtkTypeUInt32 object);

  /**
   * @brief Starts an event loop for a VTK object.
   * @param object The handle of the VTK object.
   * @return True if the event loop was successfully started, false otherwise.
   */
  bool StartEventLoop(vtkTypeUInt32 object);

  /**
   * @brief Stops an event loop for a VTK object.
   * @param object The handle of the VTK object.
   * @return True if the event loop was successfully stopped, false otherwise.
   */
  bool StopEventLoop(vtkTypeUInt32 object);

  /**
   * @brief Binds a render window to a VTK object.
   * @param object The handle of the VTK object.
   * @param canvasSelector The CSS selector for the canvas element.
   * @return True if the render window was successfully bound, false otherwise.
   */
  bool BindRenderWindow(vtkTypeUInt32 object, const std::string canvasSelector);

  /**
   * @brief Observes an event on a VTK object.
   * @param object The handle of the VTK object.
   * @param eventName The name of the event to observe.
   * @param jsFunction A JavaScript function to call when the event occurs.
   * @return A unique tag identifying the observer.
   */
  unsigned long Observe(
    vtkTypeUInt32 object, const std::string& eventName, emscripten::val jsFunction);

  /**
   * @brief Removes an observer from a VTK object.
   * @param object The handle of the VTK object.
   * @param tag The unique tag identifying the observer.
   * @return True if the observer was successfully removed, false otherwise.
   */
  bool UnObserve(vtkTypeUInt32 object, unsigned long tag);

  /**
   * @brief Exports states into `fileName.states.json` and blobs into
   * `fileName.blobs.json`.
   * @param fileName The name of the file.
   */
  void Export(const std::string& fileName);

  /**
   * @brief Imports states and blobs from files.
   * @param stateFileName The name of the file containing the states.
   * @param blobFileName The name of the file containing the blobs.
   */
  void Import(const std::string& stateFileName, const std::string& blobFileName);

  /**
   * @brief Updates all VTK objects from their corresponding states.
   */
  void UpdateObjectsFromStates();

  /**
   * @brief Updates all states from their corresponding VTK objects.
   */
  void UpdateStatesFromObjects();

  /**
   * @brief Removes unused blobs from the session.
   */
  void PruneUnusedBlobs();

  /**
   * @brief Removes unused VTK objects from the session.
   */
  void PruneUnusedObjects();

  /**
   * @brief Removes unused states from the session.
   */
  void PruneUnusedStates();

  /**
   * @brief Clears all states, objects, and blobs from the session.
   */
  void Clear();

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
