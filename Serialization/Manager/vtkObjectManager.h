// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkObjectManager
 * @brief   vtkObjectManager maintains internal instances of vtkSerializer and a vtkDeserializer to
 * serialize and deserialize VTK objects respectively.
 *
 * The vtkObjectManager facilitates:
 *  1. serialization of objects by registering them, updating their state, and providing methods to
 * retrieve both the serialized data (blobs) and object states based on their unique identifiers.
 *  2. deserialization of objects by registering their states and data (blobs) and constructing or
 * updating VTK objects based on MTime.
 *
 * @sa vtkObjectManager
 */
#ifndef vtkObjectManager_h
#define vtkObjectManager_h

#include "vtkObject.h"

#include "vtkSerializationManagerModule.h" // for export macro

#include "vtkDeserializer.h"    // for vtkDeserializer
#include "vtkInvoker.h"         // for vtkInvoker
#include "vtkLogger.h"          // for vtkLogger::Verbosity enum
#include "vtkNew.h"             // for vtkNew
#include "vtkSerializer.h"      // for vtkSerializer
#include "vtkSession.h"         // for vtkSessionObjectManagerRegistrarFunc
#include "vtkSmartPointer.h"    // for vtkSmartPointer
#include "vtkTypeUInt32Array.h" // for vtkTypeUInt32Array

#include <string> // for string
#include <vector> // for vector

VTK_ABI_NAMESPACE_BEGIN
class vtkMarshalContext;
class vtkTypeUInt8Array;

class VTKSERIALIZATIONMANAGER_EXPORT vtkObjectManager : public vtkObject
{
public:
  static vtkObjectManager* New();
  vtkTypeMacro(vtkObjectManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Loads the default (de)serialization handlers and constructors for VTK classes
   */
  virtual bool Initialize();
  bool InitializeDefaultHandlers();
  ///@}

  /**
   * Loads user provided handlers
   */
#if !defined(__VTK_WRAP__)
  bool InitializeExtensionModuleHandlers(
    const std::vector<vtkSessionObjectManagerRegistrarFunc>& registrars);
  bool InitializeExtensionModuleHandlers(
    const vtkSessionObjectManagerRegistrarFunc* registrars, std::size_t count);
#endif

  /**
   * Loads user provided handler. This is a convenience method for python bindings.
   * Users can call it with `moduleName.RegisterClasses_moduleName` argument for a module named
   * `moduleName`.
   *
   * Here is an example (assume that vtkInteractionWidgets is built outside VTK):
   *
   * ```python
   * from vtkmodules import vtkInteractionWidgets
   * from vtkmodules.vtkSerializationManager import vtkObjectManager
   * object_manager = vtkObjectManager()
   * object_manager.InitializeExtensionModuleHandler(vtkInteractionWidgets.RegisterClasses_vtkInteractionWidgets)
   * ```
   */
  bool InitializeExtensionModuleHandler(void* registrar);

  /**
   * Adds `object` into an internal container and returns a unique identifier.
   * The identifier can be used in any of the methods that accept `id` or a vector of `id`.
   */
  vtkTypeUInt32 RegisterObject(vtkSmartPointer<vtkObjectBase> objectBase);

  /**
   * Removes an object and it's state.
   * Returns true if an object exists at `id` and it was removed, false otherwise.
   */
  bool UnRegisterObject(vtkTypeUInt32 identifier);

  ///@{
  /**
   * Adds `state` into an internal container and returns a unique identifier.
   * The state
   *  1. must be valid json.
   *  2. must have a key-value pair `{'Id': n}` where n is an integer of type `std::string`.
   */
  bool RegisterState(const std::string& state);
  bool RegisterState(const nlohmann::json& state);
  ///@}

  /**
   * Removes a state at `id`.
   */
  bool UnRegisterState(vtkTypeUInt32 identifier);

  /**
   * Get the identifier for `object`.
   * Returns an integer >=0 if `object` was previously registered directly or indirectly i.e, as a
   * dependency of another registered object.
   */
  vtkTypeUInt32 GetId(vtkSmartPointer<vtkObjectBase> objectBase);

  /**
   * Get state of the object at `id`.
   * Returns a non empty json valid string if an object registered directly or indirectly at `id`
   * has a state.
   */
  std::string GetState(vtkTypeUInt32 id);

  /**
   * Get object at `id`.
   * Returns `nullptr` if there is no object registered directly or indirectly at `id`.
   */
  vtkSmartPointer<vtkObjectBase> GetObjectAtId(vtkTypeUInt32 id);

  ///@{
  /**
   * Returns a non-empty vector of identifiers of all objects that depend on an object with the
   * given identifier. Returns an empty vector if there are no dependents.
   * When the root string is empty, the entire dependency tree is returned as a flat vector of
   * identifiers.
   * The overload which returns a vector<vtkTypeUInt32> is convenient for Python bindings.
   * The overload which returns a vtkTypeUInt32Array is convenient for C
   * bindings that can take ownership of memory from the vtkAOSDataArrayTemplate.
   */
  std::vector<vtkTypeUInt32> GetAllDependencies(vtkTypeUInt32 identifier);
  vtkSmartPointer<vtkTypeUInt32Array> GetAllDependenciesAsVTKDataArray(vtkTypeUInt32 identifier);
  ///@}

  /**
   * Returns a non-empty vector of hash strings that correspond to blobs used by the registered
   * objects at each identifier in `ids`.
   */
  std::vector<std::string> GetBlobHashes(const std::vector<vtkTypeUInt32>& ids);

  /**
   * Returns a blob stored at `hash`.
   * If `copy` is `true`, a copy of the blob is returned.
   * If `copy` is `false`, the blob pointer is set in the array using `vtkTypeUInt8Array::SetArray`
   * with the save flag set to `1`.
   */
  vtkSmartPointer<vtkTypeUInt8Array> GetBlob(const std::string& hash, bool copy = false) const;

  /**
   * Specifies a `blob` for `hash`. Returns `true` if the `blob` is valid and successfully
   * registered, `false` otherwise.
   */
  bool RegisterBlob(const std::string& hash, vtkSmartPointer<vtkTypeUInt8Array> blob);

  /**
   * Removes a `blob` stored at `hash`.
   */
  bool UnRegisterBlob(const std::string& hash);

  /**
   * Removes all `blob`(s) whose `hash` is not found in the state of any object registered directly
   * or indirectly.
   */
  void PruneUnusedBlobs();

  /**
   * Deserialize registered states into vtk objects.
   */
  void UpdateObjectsFromStates();

  /**
   * Serialize registered objects into states.
   */
  void UpdateStatesFromObjects();

  /**
   * This method is similar to `void UpdateStatesFromObjects()`. The only difference is that this
   * method is far more efficient when updating a specific object and it's dependencies. The
   * identifiers must be valid and correspond to registered objects.
   *
   * @warning This method prunes all unused states and objects after serialization. Ensure that
   * `void UpdateStatesFromObjects()` is called atleast once before this method if you want to
   * preserve objects that were registered but not specified in `identifiers`.
   */
  void UpdateStatesFromObjects(const std::vector<vtkTypeUInt32>& identifiers);

  ///@{
  /**
   * Deserialize the state into vtk object.
   */
  void UpdateObjectFromState(const std::string& state);
  void UpdateObjectFromState(const nlohmann::json& state);
  ///@}

  /**
   * Serialize object at `identifier` into the state.
   */
  void UpdateStateFromObject(vtkTypeUInt32 identifier);

  /**
   * Reset to initial state.
   * All registered objects are removed and no longer tracked.
   * All registered states are also removed.
   * All registered blobs are also removed.
   */
  void Clear();

  std::string Invoke(
    vtkTypeUInt32 identifier, const std::string& methodName, const std::string& args);
  nlohmann::json Invoke(
    vtkTypeUInt32 identifier, const std::string& methodName, const nlohmann::json& args);

  std::size_t GetTotalBlobMemoryUsage();
  std::size_t GetTotalVTKDataObjectMemoryUsage();

  /**
   * Writes state of all registered objects to `filename.states.json`
   * The blobs are written into `filename.blobs.json`.
   */
  void Export(const std::string& filename, int indentLevel = -1, char indentChar = ' ');

  /**
   * Reads state from state file and blobs from blob file.
   * This clears existing states, objects, blobs, imports data from the two files and updates
   * objects from the states.
   */
  void Import(const std::string& stateFileName, const std::string& blobFileName);

  /**
   * Removes all states whose corresponding objects no longer exist.
   */
  void PruneUnusedStates();

  /**
   * Removes all objects that are neither referenced by this manager or any other object.
   */
  void PruneUnusedObjects();

  static vtkTypeUInt32 ROOT() { return 0; }

  vtkGetSmartPointerMacro(Serializer, vtkSerializer);
  vtkGetSmartPointerMacro(Deserializer, vtkDeserializer);
  vtkGetSmartPointerMacro(Invoker, vtkInvoker);

  ///@{
  /**
   * Set/Get the log verbosity of messages that are emitted when data is uploaded to GPU memory.
   * The GetObjectManagerLogVerbosity looks up system environment for
   * `VTK_OBJECT_MANAGER_LOG_VERBOSITY` that shall be used to set initial logger verbosity. The
   * default value is TRACE.
   *
   * Accepted string values are OFF, ERROR, WARNING, INFO, TRACE, MAX, INVALID or ASCII
   * representation for an integer in the range [-9,9].
   *
   * @note This method internally uses vtkLogger::ConvertToVerbosity(const char*) to parse the
   * value from environment variable.
   */
  void SetObjectManagerLogVerbosity(vtkLogger::Verbosity verbosity);
  vtkLogger::Verbosity GetObjectManagerLogVerbosity();
  ///@}

protected:
  vtkObjectManager();
  ~vtkObjectManager() override;

  vtkSmartPointer<vtkMarshalContext> Context;
  vtkNew<vtkDeserializer> Deserializer;
  vtkNew<vtkSerializer> Serializer;
  vtkNew<vtkInvoker> Invoker;
  vtkLogger::Verbosity ObjectManagerLogVerbosity = vtkLogger::VERBOSITY_INVALID;

  static const char* OWNERSHIP_KEY() { return "manager"; }

private:
  vtkObjectManager(const vtkObjectManager&) = delete;
  void operator=(const vtkObjectManager&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
