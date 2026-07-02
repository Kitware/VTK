// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCommonInformationKeyManager.h"

#include "vtkInformationKey.h"
#include "vtkInformationKeyLookup.h"

#include <algorithm>
#include <mutex>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
// Subclass vector so we can directly call constructor.  This works
// around problems on Borland C++.  Holds the mutex inline so it
// shares lifetime with the vector and is destroyed in ClassFinalize.
struct vtkCommonInformationKeyManagerKeysType : public std::vector<vtkInformationKey*>
{
  typedef std::vector<vtkInformationKey*> Superclass;
  typedef Superclass::iterator iterator;
  std::mutex Mutex;
};

//------------------------------------------------------------------------------
// Must NOT be initialized.  Default initialization to zero is
// necessary.
static unsigned int vtkCommonInformationKeyManagerCount;
static vtkCommonInformationKeyManagerKeysType* vtkCommonInformationKeyManagerKeys;

//------------------------------------------------------------------------------
vtkCommonInformationKeyManager::vtkCommonInformationKeyManager()
{
  if (++vtkCommonInformationKeyManagerCount == 1)
  {
    vtkCommonInformationKeyManager::ClassInitialize();
  }
}

//------------------------------------------------------------------------------
vtkCommonInformationKeyManager::~vtkCommonInformationKeyManager()
{
  if (--vtkCommonInformationKeyManagerCount == 0)
  {
    vtkCommonInformationKeyManager::ClassFinalize();
  }
}

//------------------------------------------------------------------------------
void vtkCommonInformationKeyManager::Register(vtkInformationKey* key)
{
  // Register this instance for deletion by the singleton.
  if (!vtkCommonInformationKeyManagerKeys)
  {
    return;
  }
  std::lock_guard<std::mutex> lock(vtkCommonInformationKeyManagerKeys->Mutex);
  vtkCommonInformationKeyManagerKeys->push_back(key);
  key->SetManagerUnregisterCallback(&vtkCommonInformationKeyManager::Unregister);
}

//------------------------------------------------------------------------------
void vtkCommonInformationKeyManager::Unregister(vtkInformationKey* key)
{
  if (!vtkCommonInformationKeyManagerKeys)
  {
    return;
  }
  std::lock_guard<std::mutex> lock(vtkCommonInformationKeyManagerKeys->Mutex);
  auto it = std::find(
    vtkCommonInformationKeyManagerKeys->begin(), vtkCommonInformationKeyManagerKeys->end(), key);
  if (it != vtkCommonInformationKeyManagerKeys->end())
  {
    vtkCommonInformationKeyManagerKeys->erase(it);
  }
}

//------------------------------------------------------------------------------
void vtkCommonInformationKeyManager::ClassInitialize()
{
  // Keep the lookup table alive at least as long as this manager,
  // since key destructors invoked from ClassFinalize unregister
  // themselves from the lookup table.
  vtkInformationKeyLookup::RetainCleanup();

  // Allocate the singleton storing pointers to information keys.
  // This must be a malloc/free pair instead of new/delete to work
  // around problems on MachO (Mac OS X) runtime systems that do lazy
  // symbol loading.  Calling operator new here causes static
  // initialization to occur in other translation units immediately,
  // which then may try to access the vector before it is set here.
  void* keys = malloc(sizeof(vtkCommonInformationKeyManagerKeysType));
  vtkCommonInformationKeyManagerKeys = new (keys) vtkCommonInformationKeyManagerKeysType;
}

//------------------------------------------------------------------------------
void vtkCommonInformationKeyManager::ClassFinalize()
{
  if (vtkCommonInformationKeyManagerKeys)
  {
    // Detach the registered keys under the lock, then delete them without
    // holding it.  Deleting a key runs its destructor, which unregisters
    // from the lookup table (a separate mutex) and, via the callback, could
    // re-enter Unregister on this manager's mutex.  Doing the deletes outside
    // our lock keeps this free of any lock-ordering or re-entrancy hazard
    // rather than relying on the callback being cleared in time.
    std::vector<vtkInformationKey*> doomedKeys;
    {
      std::lock_guard<std::mutex> lock(vtkCommonInformationKeyManagerKeys->Mutex);
      doomedKeys.swap(*vtkCommonInformationKeyManagerKeys);
    }
    for (vtkInformationKey* key : doomedKeys)
    {
      key->SetManagerUnregisterCallback(nullptr);
      delete key;
    }

    // Delete the singleton storing pointers to information keys.  See
    // ClassInitialize above for why this is a free instead of a
    // delete.
    vtkCommonInformationKeyManagerKeysType* doomed = vtkCommonInformationKeyManagerKeys;
    vtkCommonInformationKeyManagerKeys = nullptr;
    doomed->~vtkCommonInformationKeyManagerKeysType();
    free(doomed);
  }

  // Release the lookup table; if no other manager still holds a
  // reference, it will be cleaned up here.
  vtkInformationKeyLookup::ReleaseCleanup();
}
VTK_ABI_NAMESPACE_END
