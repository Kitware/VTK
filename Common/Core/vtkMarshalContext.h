// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMarshalContext
 * @brief   Shared context used by `vtkSerializer` and `vtkDeserializer`
 *
 * This class is capable of tracking dependencies among VTK objects, their states
 * and preventing recursion when the VTK serialization classes are used.
 *
 * It also provides centralized storage and tracking of objects in a weak object map,
 * recording ownership of objects using a strong object map, hashing the contents of
 * blobs to minimize data redundancies in the state and finally an API to coordinate
 * the registration and removal of states, objects and blobs.
 */
#ifndef vtkMarshalContext_h
#define vtkMarshalContext_h

#include "vtkObject.h"

#include "vtkCommonCoreModule.h" // for export macro
#include "vtkSmartPointer.h"     // for vtkSmartPointer
#include "vtkTypeUInt8Array.h"   // for vtkTypeUInt8Array
#include "vtkWeakPointer.h"      // for vtkWeakPointer

// clang-format off
#include "vtk_nlohmannjson.h"        // for json
#include VTK_NLOHMANN_JSON(json.hpp) // for json
// clang-format on

#include <memory> // for unique_ptr
#include <set>    // for set

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkMarshalContext : public vtkObject
{
public:
  vtkTypeMacro(vtkMarshalContext, vtkObject);
  static vtkMarshalContext* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using WeakObjectStore = std::map<vtkTypeUInt32, vtkWeakPointer<vtkObjectBase>>;
  using StrongObjectStore = std::map<std::string, std::set<vtkSmartPointer<vtkObjectBase>>>;

  /**
   * Get blobs.
   */
  const nlohmann::json& Blobs() const;

  /**
   * Get states.
   */
  const nlohmann::json& States() const;

  /**
   * Get map of weak objects.
   */
  const WeakObjectStore& WeakObjects() const;

  /**
   * Get map of strong objects.
   */
  const StrongObjectStore& StrongObjects() const;

  /**
   * This method creates a record of ownership between
   * `owner` and `objectBase`.
   */
  void KeepAlive(const std::string& owner, vtkObjectBase* objectBase);

  /**
   * Release the strong reference kept by `owner`
   * to the given `objectBase`.
   *
   * This method does nothing if the records show
   * that `owner` doesn't own `objectBase`.
   */
  void Retire(const std::string& owner, vtkObjectBase* objectBase);

  /**
   * Add a `state`.
   * Returns `true` if the `state` was registered, `false` otherwise.
   *
   * @note
   * The `state` is successfully registered only if a key named
   * "Id" exists in `state` and it's value is an unsigned integer.
   */
  bool RegisterState(nlohmann::json state);

  /**
   * Removes a `state`.
   * Returns `true` if a `state` exists at `identifier` and it was removed,
   * `false` otherwise.
   */
  bool UnRegisterState(vtkTypeUInt32 identifier);

  /**
   * Find and get the `state` registered at `identifier`.
   * Returns an empty `json` object if there is no `state` registered
   * at `identifier`.
   */
  nlohmann::json& GetState(vtkTypeUInt32 identifier) const;

  /**
   * Add an `objectBase` into the weak object store associated with `identifier`.
   * If `identifier` is 0, a new `identifier` will be created.
   * Returns `true` if the `objectBase` was registered, `false` otherwise.
   *
   * @note
   * The `objectBase` is successfully registered only if it is non-null.
   */
  bool RegisterObject(vtkObjectBase* objectBase, vtkTypeUInt32& identifier);

  /**
   * Removes an `objectBase` registered at `identifier` from the weak object store.
   * Returns `true` if a `objectBase` exists at `identifier` and it was removed,
   * `false` otherwise.
   */
  bool UnRegisterObject(vtkTypeUInt32 identifier);

  /**
   * Find and get the `objectBase` registered at `identifier`.
   */
  vtkSmartPointer<vtkObjectBase> GetObjectAtId(vtkTypeUInt32 identifier) const;

  /**
   * Get the `identifier` associated with `objectBase`.
   */
  vtkTypeUInt32 GetId(vtkObjectBase* objectBase) const;

  /**
   * Add a `blob` into the blob store associated with `hash`.
   * If `hash` is an empty string, the contents of `blob` will
   * be hashed and the result of the hashing algorithm stored in `hash`.
   * Returns `true` if the `blob` was registered, `false` otherwise.
   *
   * @note
   * The `blob` is successfully registered only if it is non-null.
   * This method accepts empty blobs.
   */
  bool RegisterBlob(vtkSmartPointer<vtkTypeUInt8Array> blob, std::string& hash);

  /**
   * Removes a `blob` registered at `hash` from the blob store.
   * Returns `true` if a `blob` exists at `hash` and it was removed,
   * `false` otherwise.
   */
  bool UnRegisterBlob(const std::string& hash);

  /**
   * Find and get the `blob` registered at `hash`.
   */
  vtkSmartPointer<vtkTypeUInt8Array> GetBlob(const std::string& hash);

  /**
   * Return all direct dependencies of the object/state registered at `identifier`.
   *
   * @note
   * This method doesn't compute the dependencies on demand. Instead it relies upon
   * the `vtkSerializer`/`vtkDeserializer` correctly using the `ScopedParentTracker` API
   * to record the genealogy of object(s)/state(s) serialized/deserialized.
   *
   * Technically, it's not a strict genealogy as it is possible to have circular dependencies.
   */
  std::vector<vtkTypeUInt32> GetDirectDependencies(vtkTypeUInt32 identifier) const;

  /**
   * Reset the dependency cache.
   */
  void ResetDirectDependencies();

  /**
   * Reset the dependency cache for the given `identifier`
   */
  void ResetDirectDependenciesForNode(vtkTypeUInt32 identifier);

  /**
   * Make a new `identifier`.
   */
  vtkTypeUInt32 MakeId();

  /**
   * Convenient to push a parent as the 'active' identifier and
   * add children to that parent when (de)serializing sub-states or
   * sub-objects.
   */
  class ScopedParentTracker
  {
    vtkMarshalContext* Context = nullptr;

  public:
    explicit ScopedParentTracker(vtkMarshalContext* context, vtkTypeUInt32 identifier)
      : Context(context)
    {
      if (this->Context)
      {
        this->Context->PushParent(identifier);
      }
    }

    ~ScopedParentTracker()
    {
      if (this->Context)
      {
        this->Context->PopParent();
      }
    }

    ScopedParentTracker(const ScopedParentTracker&) = delete;
    void operator=(const ScopedParentTracker&) = delete;
    ScopedParentTracker(ScopedParentTracker&&) = delete;
    void operator=(ScopedParentTracker&&) = delete;
  };

  /**
   * Make `identifier` the active identifier.
   * All subsequent `AddChild(child)` will add `child`
   * into the list of children for `identifier` until
   * `PopParent` gets invoked.
   */
  void PushParent(vtkTypeUInt32 identifier);

  /**
   * Adds the 'active' identifier into the genealogy. It's children
   * are populated using the recorded children of the 'active' `identifier`
   * using `AddChild`
   * The active parent is reset to the previous `identifier`.
   */
  void PopParent();

protected:
  vtkMarshalContext();
  ~vtkMarshalContext() override;

  /**
   * Return `true` if `objectBase` exists in the weak object store, `false` otherwise
   * If the `objectBase` exists, it's id will be stored in `identifier`.
   * @note
   * This is just a convenient method that wraps around `GetId`.
   */
  bool HasId(vtkObjectBase* objectBase, vtkTypeUInt32& identifier);

  /**
   * Return `true` if `identifier` has been through `PushParent`, but not yet
   * been through `PopParent`, false otherwise.
   */
  bool IsProcessing(vtkTypeUInt32 identifier);

  /**
   * Returns`true` if `identifier` exists in the genealogy of object(s)/state(s),
   * false otherwise.
   */
  bool IsProcessed(vtkTypeUInt32 identifier);

  /**
   * Records `identifier` in the list of children of the 'active' identifier.
   *
   * @note
   * 1. This parent-child relationship is not committed into the genealogy until
   * `PopParent` is invoked.
   *
   * 2. This method does nothing if there is no 'active' identifier, i.e,
   * when `PushParent(identifier)` was never called.
   */
  void AddChild(vtkTypeUInt32 identifier);

private:
  vtkMarshalContext(const vtkMarshalContext&) = delete;
  void operator=(const vtkMarshalContext&) = delete;

  friend class vtkDeserializer;
  friend class vtkSerializer;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
