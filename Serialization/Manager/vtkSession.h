// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file vtkSession.h
 * @brief This header file provides the C API for working with standalone/remote visualization
 * applications.
 *
 * It includes functions for creating and freeing sessions, managing
 * serialization handlers, registering and unregistering objects and states, handling blobs,
 * invoking methods, and managing dependencies. Additionally, it provides utilities for rendering,
 * event handling, pruning unused resources, and logging verbosity control.
 *
 * The API is designed to facilitate serialization and deserialization of VTK objects and states
 * in a session-based environment. It supports JSON-based state management and provides mechanisms
 * for interacting with VTK objects, including rendering and event loop management.
 *
 * Parts of this API are designed to be used in a standalone visualization application, while other
 * parts are intended for use in a remote visualization context. The API is designed to be flexible.
 *
 * @note This API provides two key capabilities:
 * - **Creating visualization pipelines directly:** Using `vtkSessionCreateObject`, users can
 *   instantiate VTK objects directly within the session, allowing for the creation of visualization
 *   pipelines in a standalone or local context. This enables users to build and manage pipelines
 *   entirely within the session without relying on external states.
 * - **Mirroring a visualization pipeline on a remote machine:** By registering states of remote
 *   objects using `vtkSessionRegisterState`, users can synchronize and manage the state of objects
 *   on a remote machine, enabling remote visualization workflows. This allows users to replicate
 *   and interact with a visualization pipeline that exists on a remote system.
 *
 * @note All sessions must be created using `vtkCreateSession` and freed using `vtkFreeSession`.
 *       Objects, states, and blobs must be registered with the session before use.
 *
 * @section  SessionUsage Using C API For VTK Sessions.
 * - Create a session using `vtkCreateSession`.
 * - Initialize the session with default or custom handlers using
 `vtkSessionInitializeObjectManager`
 *   or `vtkSessionInitializeObjectManagerExtensionHandlers`.
 * - Create objects, or register states and blobs as needed based on whether you are in a standalone
 or remote setup.
 * - Perform operations such as invoking methods, rendering, or managing dependencies.
 * - Free resources and clear the session when done.
 *
 * @section SessionDependencies Retreiving Object Dependencies
 * - The API relies on JSON for state representation and requires valid JSON structures for
 *   registering and updating states. As of now, the `vtkSessionJsonImpl` is only implemented for
 *   javascript JSON objects in the VTK::WebAssembly module.
 * - States must adhere to specific key-value pair requirements for proper registration
 *   and management.
 *
 * @section SessionLoggingVerbosity Configuring Logger Verbosity
 * - Logging verbosity can be controlled for various components of the session, including the
 *   deserializer, invoker, object manager, and serializer.
 *
 * @section SessionMemoryManagement Memory Management In VTK Session.
 * - Memory for registered objects, states, and blobs is managed by the session. Users must not
 *   manually free pointers returned by the API unless explicitly stated.
 *
 * @section SessionRendering Rendering in VTK Session
 * - Rendering-related functions are limited to specific VTK object types, such as
 *   `vtkRenderWindow`,  `vtkRenderer`, and `vtkRenderWindowInteractor`.
 *
 * @section SessionEventCallbacks Observing VTK Events With Callbacks
 * - Observers can be added to objects for specific events, and callbacks can be registered to
 *   handle these events.
 *
 * @section SessionPruning Pruning the VTK Session
 * - Unused objects, states, and blobs can be pruned from the session to optimize memory usage.
 *
 * @section SessionImportExport Import and Export a VTK Session
 * - Sessions can be imported from state and blob files, and registered objects can be serialized
 *   back into states.
 */

#ifndef vtkSession_h
#define vtkSession_h

#include <stddef.h>
#include <stdint.h>

#include "vtkABINamespace.h"

#include "vtkSerializationManagerModule.h" // for export macro

typedef uint32_t vtkObjectHandle;
typedef int vtkSessionResult;

// Forward declarations
// Opaque type representing the session implementation. See vtkSession.cxx for
// details.
typedef struct vtkSessionImpl* vtkSession;
// Opaque type representing the JSON implementation. This must be
// implemented by callers of this API.
// Look in vtkWebAssemblySessionHelper.h for an example implementation
// that supports JavaScript JSON.
typedef struct vtkSessionJsonImpl* vtkSessionJson;

// Registration function type for object manager extensions.
typedef int (*vtkSessionObjectManagerRegistrarFunc)(
  void* ser, void* deser, void* invoker, const char** error);
// JSON parsing function used to prepare a vtkSessionJson object from a JSON string.
typedef vtkSessionJson (*vtkSessionJsonParseFunc)(const char*);
// JSON stringification function used to convert a vtkSessionJson object to a JSON string.
// The caller is responsible for freeing the returned string using free() or delete[].
typedef char* (*vtkSessionJsonStringifyFunc)(vtkSessionJson);
// Callback function type for session observers.
typedef void (*vtkSessionObserverCallbackFunc)(vtkObjectHandle, const char*);

// Session descriptor structure used to initialize a session.
// It contains function pointers for JSON parsing and stringification, as well as options for event
// loop management.
typedef struct vtkSessionDescriptor
{
  vtkSessionJsonParseFunc ParseJson;
  vtkSessionJsonStringifyFunc StringifyJson;
  int InteractorManagesTheEventLoop;
} vtkSessionDescriptor;

#define vtkSessionResultSuccess 1
#define vtkSessionResultFailure 0

// clang-format off
#define vtkCreateSession VTK_ABI_NAMESPACE_MANGLE(vtkCreateSession)
#define vtkFreeSession VTK_ABI_NAMESPACE_MANGLE(vtkFreeSession)
#define vtkSessionInitializeObjectManager VTK_ABI_NAMESPACE_MANGLE(vtkSessionInitializeObjectManager)
#define vtkSessionInitializeObjectManagerExtensionHandlers VTK_ABI_NAMESPACE_MANGLE(vtkSessionInitializeObjectManagerExtensionHandlers)
#define vtkSessionGetManager VTK_ABI_NAMESPACE_MANGLE(vtkSessionGetManager)
#define vtkSessionCreateObject VTK_ABI_NAMESPACE_MANGLE(vtkSessionCreateObject)
#define vtkSessionDestroyObject VTK_ABI_NAMESPACE_MANGLE(vtkSessionDestroyObject)
#define vtkSessionRegisterState VTK_ABI_NAMESPACE_MANGLE(vtkSessionRegisterState)
#define vtkSessionUnRegisterState VTK_ABI_NAMESPACE_MANGLE(vtkSessionUnRegisterState)
#define vtkSessionGetState VTK_ABI_NAMESPACE_MANGLE(vtkSessionGetState)
#define vtkSessionSkipProperty VTK_ABI_NAMESPACE_MANGLE(vtkSessionSkipProperty)
#define vtkSessionUnSkipProperty VTK_ABI_NAMESPACE_MANGLE(vtkSessionUnSkipProperty)
#define vtkSessionRegisterBlob VTK_ABI_NAMESPACE_MANGLE(vtkSessionRegisterBlob)
#define vtkSessionUnRegisterBlob VTK_ABI_NAMESPACE_MANGLE(vtkSessionUnRegisterBlob)
#define vtkSessionGetBlob VTK_ABI_NAMESPACE_MANGLE(vtkSessionGetBlob)
#define vtkSessionPruneUnusedBlobs VTK_ABI_NAMESPACE_MANGLE(vtkSessionPruneUnusedBlobs)
#define vtkSessionPruneUnusedObjects VTK_ABI_NAMESPACE_MANGLE(vtkSessionPruneUnusedObjects)
#define vtkSessionPruneUnusedStates VTK_ABI_NAMESPACE_MANGLE(vtkSessionPruneUnusedStates)
#define vtkSessionClear VTK_ABI_NAMESPACE_MANGLE(vtkSessionClear)
#define vtkSessionInvoke VTK_ABI_NAMESPACE_MANGLE(vtkSessionInvoke)
#define vtkSessionGetAllDependencies VTK_ABI_NAMESPACE_MANGLE(vtkSessionGetAllDependencies)
#define vtkSessionFreeGetAllDependenciesResult VTK_ABI_NAMESPACE_MANGLE(vtkSessionFreeGetAllDependenciesResult)
#define vtkSessionGetTotalBlobMemoryUsage VTK_ABI_NAMESPACE_MANGLE(vtkSessionGetTotalBlobMemoryUsage)
#define vtkSessionGetTotalVTKDataObjectMemoryUsage VTK_ABI_NAMESPACE_MANGLE(vtkSessionGetTotalVTKDataObjectMemoryUsage)
#define vtkSessionUpdateObjectsFromStates VTK_ABI_NAMESPACE_MANGLE(vtkSessionUpdateObjectsFromStates)
#define vtkSessionUpdateStatesFromObjects VTK_ABI_NAMESPACE_MANGLE(vtkSessionUpdateStatesFromObjects)
#define vtkSessionUpdateObjectFromState VTK_ABI_NAMESPACE_MANGLE(vtkSessionUpdateObjectFromState)
#define vtkSessionUpdateStateFromObject VTK_ABI_NAMESPACE_MANGLE(vtkSessionUpdateStateFromObject)
#define vtkSessionSetSize VTK_ABI_NAMESPACE_MANGLE(vtkSessionSetSize)
#define vtkSessionRender VTK_ABI_NAMESPACE_MANGLE(vtkSessionRender)
#define vtkSessionResetCamera VTK_ABI_NAMESPACE_MANGLE(vtkSessionResetCamera)
#define vtkSessionStartEventLoop VTK_ABI_NAMESPACE_MANGLE(vtkSessionStartEventLoop)
#define vtkSessionStopEventLoop VTK_ABI_NAMESPACE_MANGLE(vtkSessionStopEventLoop)
#define vtkSessionAddObserver VTK_ABI_NAMESPACE_MANGLE(vtkSessionAddObserver)
#define vtkSessionRemoveObserver VTK_ABI_NAMESPACE_MANGLE(vtkSessionRemoveObserver)
#define vtkSessionImport VTK_ABI_NAMESPACE_MANGLE(vtkSessionImport)
#define vtkSessionExport VTK_ABI_NAMESPACE_MANGLE(vtkSessionExport)
#define vtkSessionPrintObjectToString VTK_ABI_NAMESPACE_MANGLE(vtkSessionPrintObjectToString)
#define vtkSessionPrintSceneManagerInformation VTK_ABI_NAMESPACE_MANGLE(vtkSessionPrintSceneManagerInformation)
#define vtkSessionSetDeserializerLogVerbosity VTK_ABI_NAMESPACE_MANGLE(vtkSessionSetDeserializerLogVerbosity)
#define vtkSessionSetInvokerLogVerbosity VTK_ABI_NAMESPACE_MANGLE(vtkSessionSetInvokerLogVerbosity)
#define vtkSessionSetObjectManagerLogVerbosity VTK_ABI_NAMESPACE_MANGLE(vtkSessionSetObjectManagerLogVerbosity)
#define vtkSessionSetSerializerLogVerbosity VTK_ABI_NAMESPACE_MANGLE(vtkSessionSetSerializerLogVerbosity)
// clang-format on

#ifdef __cplusplus
extern "C"
#endif
{
  /**
   * Create a session
   * @param descriptor The descriptor is used to initialize the session. It contains the function
   * pointers to parse and stringify JSON and other options.
   * @return A session object. The caller is responsible for freeing the session using
   * vtkFreeSession.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSession vtkCreateSession(
    const vtkSessionDescriptor* descriptor);

  /**
   * Free a session
   * @param session The session to free. The session must be created using vtkCreateSession.
   * @note The session must be freed using this function. Do not use `delete` or `free` to free the
   * session.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkFreeSession(vtkSession session);

  /**
   * Loads the default (de)serialization handlers and constructors for VTK classes
   * @param session The session to initialize. The session must be created using vtkCreateSession.
   * @return A vtkSessionResult indicating success or failure.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult vtkSessionInitializeObjectManager(
    vtkSession session);

  /**
   * Loads user provided handlers
   * @param session The session to initialize. The session must be created using vtkCreateSession.
   * @param registrars The array of function pointers to register the handlers.
   * @param count The number of function pointers in the array.
   * @return A vtkSessionResult indicating success or failure.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult
  vtkSessionInitializeObjectManagerExtensionHandlers(
    vtkSession session, const vtkSessionObjectManagerRegistrarFunc* registrars, size_t count);

  /**
   * Get underlying object manager of a vtkSession.
   * @param session The session to get the object manager from. The session must be created using
   * vtkCreateSession.
   * @return A pointer to the object manager.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void* vtkSessionGetManager(vtkSession session);

  /**
   * Create an object of type `className`.
   * @param session The session to create the object in. The session must be created using
   * vtkCreateSession.
   * @param className The name of the class to create.
   * @return A vtkObjectHandle that can be used to access the object. The object is registered with
   * the session and can be used in any of the methods that accept a `vtkObjectHandle`. The caller
   * is responsible for freeing the object using vtkSessionDestroyObject.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkObjectHandle vtkSessionCreateObject(
    vtkSession session, const char* className);

  /**
   * Destroy a VTKObject
   * @param session The session to destroy the object in. The session must be created using
   * vtkCreateSession.
   * @param object The object to destroy. The object must be created using
   * vtkSessionRegisterState or vtkSessionCreateObject or a dependent of objects created
   * through either of those two methods.
   * @return A vtkSessionResult indicating success or failure.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult vtkSessionDestroyObject(
    vtkSession session, vtkObjectHandle object);

  /**
   * Adds `state` into an internal container and returns a unique identifier.
   *
   * @param session The session to register the state in. The session must be created using
   * vtkCreateSession.
   * @param state The state to register. The state must be valid json and must have the required key
   * value pairs.
   * @return A vtkSessionResult indicating success or failure.
   * @note The state must be valid json and must have the required key value pairs. The state must
   *  1. be valid json.
   *  2. have a key-value pair `{'Id': n}` where n is an integer of type vtkObjectHandle
   *  3. have a key-value pair `{'ClassName': "className"}` where "className" is the name of the
   *    class.
   *  4. have a key-value pair `{'Superclass': ["superClassName1", ..]}` where "superClassName1" is
   *    the name of a superclass. The superclass names must be ordered from the least derived to the
   *    most derived class. The superclass names must be valid class names.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult vtkSessionRegisterState(
    vtkSession session, vtkSessionJson state);

  /**
   * Removes a state at `id`.
   * @param session The session to unregister the state from. The session must be created using
   * vtkCreateSession.
   * @param object The object to unregister. The object must be registered using
   * vtkSessionRegisterState or vtkSessionCreateObject or a dependent of objects created
   * through either of those two methods.
   * @return A vtkSessionResult indicating success or failure.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult vtkSessionUnRegisterState(
    vtkSession session, vtkObjectHandle object);

  /**
   * Get the state of the object at `id`.
   * @param session The session to get the state from. The session must be created using
   * vtkCreateSession.
   * @param object The object to get the state from. The object must be registered using
   * vtkSessionRegisterState or vtkSessionCreateObject or a dependent of objects created
   * through either of those two methods.
   * @return A vtkSessionJson that can be used to access the state. The state is registered with the
   * session and can be used in any of the methods that accept a `vtkSessionJson`.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionJson vtkSessionGetState(
    vtkSession session, vtkObjectHandle object);

  /**
   * Skip a property of a class. The property will not be serialized or deserialized.
   * @param session The session to skip the property in. The session must be created using
   * vtkCreateSession.
   * @param className The name of the class to skip the property in.
   * @param propertyName The name of the property to skip.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionSkipProperty(
    vtkSession session, const char* className, const char* propertyName);

  /**
   * Remove a property of a class from the skip list. The property will be serialized and
   * deserialized.
   * @param session The session to unskip the property in. The session must be created using
   * vtkCreateSession.
   * @param className The name of the class to unskip the property in.
   * @param propertyName The name of the property to unskip.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionUnSkipProperty(
    vtkSession session, const char* className, const char* propertyName);

  /**
   * Register a blob with the session.
   * @param session The session to register the blob with. The session must be created using
   * vtkCreateSession.
   * @param hash The hash of the blob. The hash must be a valid string.
   * @param blob The blob to register. The blob must be a valid pointer to a uint8_t array.
   * @param length The length of the blob. The length must be a valid size_t.
   * @return A vtkSessionResult indicating success or failure.
   * @note The blob gets owned by the session and will be freed when the blob is unregistered with
   * the vtkSessionUnRegisterBlob function or when the session is destroyed using vtkFreeSession.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult vtkSessionRegisterBlob(
    vtkSession session, const char* hash, uint8_t* blob, size_t length);

  /**
   * Unregister a blob with the session.
   * @param session The session to unregister the blob from. The session must be created using
   * vtkCreateSession.
   * @param hash The hash of the blob. The hash must be a valid string.
   * @return A vtkSessionResult indicating success or failure.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult vtkSessionUnRegisterBlob(
    vtkSession session, const char* hash);

  /**
   * Get a blob from the session.
   * @param session The session to get the blob from. The session must be created using
   * vtkCreateSession.
   * @param hash The hash of the blob. The hash must be a valid string.
   * @param length A pointer to a size_t that will be set to the length of the blob.
   * @return A pointer to the blob. The caller should never free the pointer. The blob is owned by
   * the session and will be freed when the session is destroyed or when the blob is unregistered
   * with the vtkSessionUnRegisterBlob function.
   */
  VTKSERIALIZATIONMANAGER_EXPORT uint8_t* vtkSessionGetBlob(
    vtkSession session, const char* hash, size_t* length);

  /**
   * Invokes a method on the object at `id` with the given arguments.
   * @param session The session to invoke the method on. The session must be created using
   * vtkCreateSession.
   * @param object The object to invoke the method on. The object must be registered using
   * vtkSessionRegisterState or vtkSessionCreateObject or a dependent of objects created
   * through either of those two methods.
   * @param methodName The name of the method to invoke. The method must be a valid string.
   * @param args The arguments to pass to the method. The arguments must be a valid vtkSessionJson.
   * @return A vtkSessionJson that can be used to access the result. If the result is an identifier,
   * it will be registered with the session and can be used in any of the methods that accept a
   * `vtkObjectHandle`.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionJson vtkSessionInvoke(
    vtkSession session, vtkObjectHandle object, const char* methodName, vtkSessionJson args);

  /**
   * Get all dependencies of an object.
   * @param session The session to get the dependencies from. The session must be created using
   * vtkCreateSession.
   * @param object The object to get the dependencies from. The object must be registered using
   * vtkSessionRegisterState or vtkSessionCreateObject or a dependent of objects created
   * through either of those two methods.
   * @param count A pointer to a size_t that will be set to the number of dependencies.
   * @return A pointer to an array of vtkObjectHandle that can be used to access the dependencies.
   * The caller should free the pointer using vtkSessionFreeGetAllDependenciesResult.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkObjectHandle* vtkSessionGetAllDependencies(
    vtkSession session, vtkObjectHandle object, size_t* count);

  /**
   * Free the result of vtkSessionGetAllDependencies.
   * @param session The session to free the result from. The session must be created using
   * vtkCreateSession.
   * @param ptr The pointer to the result to free. The pointer must be a valid pointer to an array
   * of vtkObjectHandle.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionFreeGetAllDependenciesResult(
    vtkSession session, vtkObjectHandle* ptr);

  /**
   * Update the object from the state.
   * @param session The session to update the object in. The session must be created using
   * vtkCreateSession.
   * @param state The state to update the object from. The state must be valid json and must have
   * the required key value pairs.
   * @return A vtkSessionResult indicating success or failure.
   * @note The state must be valid json and must have the required key value pairs. The state must
   *  1. be valid json.
   *  2. have atleast a key-value pair `{'Id': n}` where n is an integer of type vtkObjectHandle
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult vtkSessionUpdateObjectFromState(
    vtkSession session, vtkSessionJson state);

  /**
   * Update the state from the object.
   * @param session The session to update the state from. The session must be created using
   * vtkCreateSession.
   * @param object The object to update the state from. The object must be registered using
   * vtkSessionRegisterState or vtkSessionCreateObject or a dependent of objects created
   * through either of those two methods.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionUpdateStateFromObject(
    vtkSession session, vtkObjectHandle object);

  /**
   * Set the size of the render window.
   * @param session The session to set the size in. The session must be created using
   * vtkCreateSession.
   * @param object The object to set the size for. The object must be registered using
   * vtkSessionRegisterState or vtkSessionCreateObject or a dependent of objects created
   * through either of those two methods.
   * @param width The width of the render window.
   * @param height The height of the render window.
   * @return A vtkSessionResult indicating success or failure.
   * @note This method is only valid for render windows. It will not work for other objects.
   * The object must be a valid vtkRenderWindow or a subclass of vtkRenderWindow.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult vtkSessionSetSize(
    vtkSession session, vtkObjectHandle object, int width, int height);

  /**
   * Render the window.
   * @param session The session to render the object in. The session must be created
   * usingvtkCreateSession.
   * @param object The object to render. The object must be registered usingvtkSessionRegisterState
   * or vtkSessionCreateObject or a dependent of objects created through either of those two
   * methods.
   * @return A vtkSessionResult indicating success or failure.
   * @note This method is only valid for render windows. It will not work for other objects.
   * The object must be a valid vtkRenderWindow or a subclass of vtkRenderWindow.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult vtkSessionRender(
    vtkSession session, vtkObjectHandle object);

  /**
   * Reset the camera of the renderer to fit the bounds of the scene.
   * @param session The session to reset the camera in. The session must be created using
   * vtkCreateSession.
   * @param object The object to reset the camera for. The object must be registered using
   * vtkSessionRegisterState or vtkSessionCreateObject or a dependent of objects created
   * through either of those two methods.
   * @return A vtkSessionResult indicating success or failure.
   * @note This method is only valid for renderers. It will not work for other objects.
   * The object must be a valid vtkRenderer or a subclass of vtkRenderer.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult vtkSessionResetCamera(
    vtkSession session, vtkObjectHandle object);

  /**
   * Start the event loop for the render window interactor.
   * @param session The session to start the event loop in. The session must be created using
   * vtkCreateSession.
   * @param object The object to start the event loop for. The object must be registered using
   * vtkSessionRegisterState or vtkSessionCreateObject or a dependent of objects created
   * through either of those two methods.
   * @return A vtkSessionResult indicating success or failure.
   * @note This method is only valid for render window interactors. It will not work for other
   * objects. The object must be a valid vtkRenderWindowInteractor or a subclass of
   * vtkRenderWindowInteractor.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult vtkSessionStartEventLoop(
    vtkSession session, vtkObjectHandle object);

  /**
   * Stop the event loop for the render window interactor.
   * @param session The session to start the event loop in. The session must be created using
   * vtkCreateSession.
   * @param object The object to start the event loop for. The object must be registered using
   * vtkSessionRegisterState or vtkSessionCreateObject or a dependent of objects created
   * through either of those two methods.
   * @return A vtkSessionResult indicating success or failure.
   * @note This method is only valid for render window interactors. It will not work for other
   * objects. The object must be a valid vtkRenderWindowInteractor or a subclass of
   * vtkRenderWindowInteractor.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult vtkSessionStopEventLoop(
    vtkSession session, vtkObjectHandle object);

  /**
   * Add an observer to the object for the given event.
   * @param session The session to add the observer to. The session must be created using
   * vtkCreateSession.
   * @param object The object to add the observer to. The object must be registered using
   * vtkSessionRegisterState or vtkSessionCreateObject or a dependent of objects created
   * through either of those two methods.
   * @param eventName The name of the event to observe. The event must be a valid string.
   * @param callback The callback function to call when the event is triggered. The callback must be
   * a valid function pointer.
   * @return A tag that can be used to remove the observer. The tag is a unique identifier for the
   * observer.
   * @note The callback function must have the following signature:
   * void callback(vtkObjectHandle sender, const char* eventName);
   */
  VTKSERIALIZATIONMANAGER_EXPORT unsigned long vtkSessionAddObserver(vtkSession session,
    vtkObjectHandle object, const char* eventName, vtkSessionObserverCallbackFunc callback);

  /**
   * Remove an observer from the object for the given event.
   * @param session The session to remove the observer from. The session must be created using
   * vtkCreateSession.
   * @param object The object to remove the observer from. The object must be registered using
   * vtkSessionRegisterState or vtkSessionCreateObject or a dependent of objects created
   * through either of those two methods.
   * @param tag The tag of the observer to remove. The tag is a unique identifier for the observer.
   * @return A vtkSessionResult indicating success or failure.
   * @note The tag must be a valid tag that was returned by vtkSessionAddObserver.
   */
  VTKSERIALIZATIONMANAGER_EXPORT vtkSessionResult vtkSessionRemoveObserver(
    vtkSession session, vtkObjectHandle object, unsigned long tag);

  /**
   * Writes state of all registered objects to `fileName.states.json`
   * The blobs are written into `fileName.blobs.json`.
   * @param session The session to export the state and blob into. The session must be created using
   * vtkCreateSession.
   * @param fileName The name of the state file to export. The file must be a valid json file.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionExport(vtkSession session, const char* fileName);

  /**
   * Import a session from a state file and a blob file.
   * @param session The session to import the state and blob into. The session must be created using
   * vtkCreateSession.
   * @param stateFileName The name of the state file to import. The file must be a valid json file.
   * @param blobFileName The name of the blob file to import. The file must be a valid json file.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionImport(
    vtkSession session, const char* stateFileName, const char* blobFileName);

  /**
   * Deserialize registered states from objects.
   * @param session The session to deserialize the states in. The session must be created using
   * vtkCreateSession.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionUpdateObjectsFromStates(vtkSession session);

  /**
   * Serialize registered objects into states.
   * @param session The session to serialize the objects in. The session must be created using
   * vtkCreateSession.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionUpdateStatesFromObjects(vtkSession session);

  /**
   * Prune unused blobs from the session.
   * @param session The session to prune the blobs from. The session must be created using
   * vtkCreateSession.
   * @note This method will remove all blobs that are not used by any object or state.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionPruneUnusedBlobs(vtkSession session);

  /**
   * Prune unused objects from the session.
   * @param session The session to prune the objects from. The session must be created using
   * vtkCreateSession.
   * @note This method will remove all objects that are not referenced by any registered state.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionPruneUnusedObjects(vtkSession session);

  /**
   * Prune unused states from the session.
   * @param session The session to prune the states from. The session must be created using
   * vtkCreateSession.
   * @note This method will remove all states that are not used by any registered object.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionPruneUnusedStates(vtkSession session);

  /**
   * Clear the session. This will remove all registered objects, states and blobs.
   * @param session The session to clear. The session must be created using vtkCreateSession.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionClear(vtkSession session);

  /**
   * Get the total memory usage of the blobs in the session.
   * @param session The session to get the memory usage from. The session must be created using
   * vtkCreateSession.
   * @return The total memory usage of the blobs in bytes.
   */
  VTKSERIALIZATIONMANAGER_EXPORT size_t vtkSessionGetTotalBlobMemoryUsage(vtkSession session);

  /**
   * Get the total memory usage of the VTK data objects in the session.
   * @param session The session to get the memory usage from. The session must be created using
   * vtkCreateSession.
   * @return The total memory usage of the VTK data objects in bytes.
   */
  VTKSERIALIZATIONMANAGER_EXPORT size_t vtkSessionGetTotalVTKDataObjectMemoryUsage(
    vtkSession session);

  /**
   * Print the object information to the console.
   * @param session The session to print the information from. The session must be created using
   * vtkCreateSession.
   * @param object The object to print the information for. The object must be registered using
   * vtkSessionRegisterState or vtkSessionCreateObject or a dependent of objects created
   * through either of those two methods.
   * @return A string containing the object information. The caller is responsible for freeing the
   * string using free() or delete[].
   */
  VTKSERIALIZATIONMANAGER_EXPORT char* vtkSessionPrintObjectToString(
    vtkSession session, vtkObjectHandle object);

  /**
   * Print the scene manager information to the console.
   * @param session The session to print the information from. The session must be created using
   * vtkCreateSession.
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionPrintSceneManagerInformation(vtkSession session);

  /**
   * Set the log verbosity for the session deserializer.
   * @param session The session to set the log verbosity for. The session must be created using
   * vtkCreateSession.
   * @param verbosityStr The verbosity level to set. The verbosity level must be a valid string.
   * The valid values are "OFF", "ERROR", "WARNING", "INFO", "TRACE", "MAX", "INVALID" or "ASCII".
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionSetDeserializerLogVerbosity(
    vtkSession session, const char* verbosityStr);

  /**
   * Set the log verbosity for the session invoker.
   * @param session The session to set the log verbosity for. The session must be created using
   * vtkCreateSession.
   * @param verbosityStr The verbosity level to set. The verbosity level must be a valid string.
   * The valid values are "OFF", "ERROR", "WARNING", "INFO", "TRACE", "MAX", "INVALID" or "ASCII".
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionSetInvokerLogVerbosity(
    vtkSession session, const char* verbosityStr);

  /**
   * Set the log verbosity for the session object manager.
   * @param session The session to set the log verbosity for. The session must be created using
   * vtkCreateSession.
   * @param verbosityStr The verbosity level to set. The verbosity level must be a valid string.
   * The valid values are "OFF", "ERROR", "WARNING", "INFO", "TRACE", "MAX", "INVALID" or "ASCII".
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionSetObjectManagerLogVerbosity(
    vtkSession session, const char* verbosityStr);

  /**
   * Set the log verbosity for the session serializer.
   * @param session The session to set the log verbosity for. The session must be created using
   * vtkCreateSession.
   * @param verbosityStr The verbosity level to set. The verbosity level must be a valid string.
   * The valid values are "OFF", "ERROR", "WARNING", "INFO", "TRACE", "MAX", "INVALID" or "ASCII".
   */
  VTKSERIALIZATIONMANAGER_EXPORT void vtkSessionSetSerializerLogVerbosity(
    vtkSession session, const char* verbosityStr);
#ifdef __cplusplus
} // extern "C"
#endif

#endif
// VTK-HeaderTest-Exclude: vtkSession.h
