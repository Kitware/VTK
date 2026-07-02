// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkInformationKeyLookup.h"

#include "vtkInformationKey.h"
#include "vtkObjectFactory.h"

#include <atomic>
#include <cassert>
#include <map>
#include <mutex>
#include <utility>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkInformationKeyLookup);

namespace
{
// Bundles the lookup map with its guarding mutex so the two share a
// lifetime and can be allocated/freed together via the same
// malloc/placement-new + dtor/free pattern used by the manager classes.
struct vtkInformationKeyLookupKeysType
{
  std::map<std::pair<std::string, std::string>, vtkInformationKey*> Map;
  std::mutex Mutex;
};

// Reference count drives lifetime: each key manager calls
// RetainCleanup() during its ClassInitialize and ReleaseCleanup()
// during its ClassFinalize.  When the count returns to zero (after
// every manager has finished destroying its keys) the lookup table
// is freed.
std::atomic<unsigned int> gRefCount{ 0 };
std::atomic<vtkInformationKeyLookupKeysType*> gKeys{ nullptr };
}

//------------------------------------------------------------------------------
void vtkInformationKeyLookup::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Registered Keys:\n";
  indent = indent.GetNextIndent();
  auto* keys = gKeys.load(std::memory_order_acquire);
  if (!keys)
  {
    return;
  }
  std::lock_guard<std::mutex> lock(keys->Mutex);
  for (auto i = keys->Map.begin(), iEnd = keys->Map.end(); i != iEnd; ++i)
  {
    os << indent << i->first.first << "::" << i->first.second << " @" << i->second << " ("
       << i->second->GetClassName() << ")\n";
  }
}

//------------------------------------------------------------------------------
vtkInformationKey* vtkInformationKeyLookup::Find(
  const std::string& name, const std::string& location)
{
  auto* keys = gKeys.load(std::memory_order_acquire);
  if (!keys)
  {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(keys->Mutex);
  auto it = keys->Map.find(std::make_pair(location, name));
  return it != keys->Map.end() ? it->second : nullptr;
}

//------------------------------------------------------------------------------
vtkInformationKey* vtkInformationKeyLookup::FindByName(const std::string& name)
{
  auto* keys = gKeys.load(std::memory_order_acquire);
  if (!keys)
  {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(keys->Mutex);
  vtkInformationKey* result = nullptr;
  for (auto it = keys->Map.begin(); it != keys->Map.end(); ++it)
  {
    if (it->first.second == name)
    {
      if (result != nullptr)
      {
        // Multiple matches — ambiguous.
        return nullptr;
      }
      result = it->second;
    }
  }
  return result;
}

//------------------------------------------------------------------------------
vtkInformationKeyLookup::vtkInformationKeyLookup() = default;

//------------------------------------------------------------------------------
// Keys are owned / cleaned up by the vtk*InformationKeyManagers.
vtkInformationKeyLookup::~vtkInformationKeyLookup() = default;

//------------------------------------------------------------------------------
void vtkInformationKeyLookup::RetainCleanup()
{
  // RetainCleanup is invoked from each key manager's ClassInitialize,
  // which runs single-threaded during static initialization / library
  // load.  That serialization is what makes the store below safe: there
  // is no concurrent caller that could observe gKeys after the fetch_add
  // but before the placement-new completes.
  if (gRefCount.fetch_add(1, std::memory_order_acq_rel) == 0)
  {
    // First retain — allocate the storage.  Use malloc/placement new
    // for the same lazy-symbol-loading reason documented in
    // vtkCommonInformationKeyManager::ClassInitialize.
    void* mem = malloc(sizeof(vtkInformationKeyLookupKeysType));
    gKeys.store(new (mem) vtkInformationKeyLookupKeysType, std::memory_order_release);
  }
}

//------------------------------------------------------------------------------
void vtkInformationKeyLookup::ReleaseCleanup()
{
  if (gRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
  {
    // Last release — destroy the storage.
    vtkInformationKeyLookupKeysType* doomed = gKeys.exchange(nullptr, std::memory_order_acq_rel);
    if (doomed)
    {
      doomed->~vtkInformationKeyLookupKeysType();
      free(doomed);
    }
  }
}

//------------------------------------------------------------------------------
void vtkInformationKeyLookup::RegisterKey(
  vtkInformationKey* key, const std::string& name, const std::string& location)
{
  // g_Keys is allocated by RetainCleanup, called from a key manager's
  // ClassInitialize.  Every key registers with a manager, and the base
  // vtkInformationKey constructor (which calls this) runs before the
  // derived key registers with its manager, so the manager's
  // ClassInitialize has already run by the time we get here.  If that
  // invariant is ever broken (e.g. a key constructed during static
  // initialization in a translation unit that pulls in no manager), the
  // key would be silently dropped from the lookup table; assert in debug
  // builds so the regression is caught rather than failing as a missing
  // string lookup at runtime.
  auto* keys = gKeys.load(std::memory_order_acquire);
  assert(keys && "vtkInformationKey registered before any key manager initialized");
  if (!keys)
  {
    return;
  }
  std::lock_guard<std::mutex> lock(keys->Mutex);
  keys->Map.insert(std::make_pair(std::make_pair(location, name), key));
}

//------------------------------------------------------------------------------
void vtkInformationKeyLookup::UnregisterKey(vtkInformationKey* key)
{
  auto* keys = gKeys.load(std::memory_order_acquire);
  if (!key || !keys)
  {
    return;
  }
  const char* name = key->GetName();
  const char* location = key->GetLocation();
  if (!name || !location)
  {
    return;
  }
  std::lock_guard<std::mutex> lock(keys->Mutex);
  auto it = keys->Map.find(std::make_pair(std::string(location), std::string(name)));
  if (it != keys->Map.end() && it->second == key)
  {
    keys->Map.erase(it);
  }
}
VTK_ABI_NAMESPACE_END
