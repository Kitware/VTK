// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFilteringInformationKeyManager.h"

#include "vtkCellMetadata.h"
#include "vtkInformationKey.h"
#include "vtkInformationKeyLookup.h"

#include <algorithm>
#include <mutex>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
std::vector<std::function<void()>>* vtkFilteringInformationKeyManager::Finalizers = nullptr;

// Subclass vector so we can directly call constructor.  This works
// around problems on Borland C++.  Holds the mutex inline so it
// shares lifetime with the vector and is destroyed in ClassFinalize.
struct vtkFilteringInformationKeyManagerKeysType : public std::vector<vtkInformationKey*>
{
  typedef std::vector<vtkInformationKey*> Superclass;
  typedef Superclass::iterator iterator;
  std::mutex Mutex;
};

//------------------------------------------------------------------------------
// Must NOT be initialized.  Default initialization to zero is
// necessary.
static unsigned int vtkFilteringInformationKeyManagerCount;
static vtkFilteringInformationKeyManagerKeysType* vtkFilteringInformationKeyManagerKeys;

//------------------------------------------------------------------------------
vtkFilteringInformationKeyManager::vtkFilteringInformationKeyManager()
{
  if (++vtkFilteringInformationKeyManagerCount == 1)
  {
    vtkFilteringInformationKeyManager::ClassInitialize();
  }
}

//------------------------------------------------------------------------------
vtkFilteringInformationKeyManager::~vtkFilteringInformationKeyManager()
{
  if (--vtkFilteringInformationKeyManagerCount == 0)
  {
    vtkFilteringInformationKeyManager::ClassFinalize();
  }
}

//------------------------------------------------------------------------------
void vtkFilteringInformationKeyManager::Register(vtkInformationKey* key)
{
  // Register this instance for deletion by the singleton.
  if (!vtkFilteringInformationKeyManagerKeys)
  {
    return;
  }
  std::lock_guard<std::mutex> lock(vtkFilteringInformationKeyManagerKeys->Mutex);
  vtkFilteringInformationKeyManagerKeys->push_back(key);
  key->SetManagerUnregisterCallback(&vtkFilteringInformationKeyManager::Unregister);
}

//------------------------------------------------------------------------------
void vtkFilteringInformationKeyManager::Unregister(vtkInformationKey* key)
{
  if (!vtkFilteringInformationKeyManagerKeys)
  {
    return;
  }
  std::lock_guard<std::mutex> lock(vtkFilteringInformationKeyManagerKeys->Mutex);
  auto it = std::find(vtkFilteringInformationKeyManagerKeys->begin(),
    vtkFilteringInformationKeyManagerKeys->end(), key);
  if (it != vtkFilteringInformationKeyManagerKeys->end())
  {
    vtkFilteringInformationKeyManagerKeys->erase(it);
  }
}

//------------------------------------------------------------------------------
void vtkFilteringInformationKeyManager::AddFinalizer(std::function<void()> finalizer)
{
  if (!vtkFilteringInformationKeyManager::Finalizers)
  {
    vtkFilteringInformationKeyManager::Finalizers = new std::vector<std::function<void()>>();
  }
  vtkFilteringInformationKeyManager::Finalizers->push_back(finalizer);
}

//------------------------------------------------------------------------------
void vtkFilteringInformationKeyManager::ClassInitialize()
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
  void* keys = malloc(sizeof(vtkFilteringInformationKeyManagerKeysType));
  vtkFilteringInformationKeyManagerKeys = new (keys) vtkFilteringInformationKeyManagerKeysType;

  // The cell-metadata class cannot register its finalizer upon construction
  // of the registrar since CommonDataModel cannot depend on CommonExecutionModel.
  // So, we always register a finalizer for cell-grid responders.
  vtkFilteringInformationKeyManager::AddFinalizer([]() { vtkCellMetadata::ClearResponders(); });
}

//------------------------------------------------------------------------------
void vtkFilteringInformationKeyManager::ClassFinalize()
{
  // Allow persistent objects to be cleaned up before debugging leaks.
  if (vtkFilteringInformationKeyManager::Finalizers)
  {
    for (const auto& finalizer : *vtkFilteringInformationKeyManager::Finalizers)
    {
      finalizer();
    }
    delete vtkFilteringInformationKeyManager::Finalizers;
    vtkFilteringInformationKeyManager::Finalizers = nullptr;
  }

  if (vtkFilteringInformationKeyManagerKeys)
  {
    // Delete information keys.  Clear the callback first so the
    // key destructor does not try to modify the vector during iteration.
    // The mutex is held while iterating; the key destructor only locks
    // the lookup table's separate mutex (via UnregisterKey), so there
    // is no nested-lock conflict on this manager's mutex.
    {
      std::lock_guard<std::mutex> lock(vtkFilteringInformationKeyManagerKeys->Mutex);
      for (vtkFilteringInformationKeyManagerKeysType::iterator i =
             vtkFilteringInformationKeyManagerKeys->begin();
           i != vtkFilteringInformationKeyManagerKeys->end(); ++i)
      {
        vtkInformationKey* key = *i;
        key->SetManagerUnregisterCallback(nullptr);
        delete key;
      }
    }

    // Delete the singleton storing pointers to information keys.  See
    // ClassInitialize above for why this is a free instead of a
    // delete.
    vtkFilteringInformationKeyManagerKeysType* doomed = vtkFilteringInformationKeyManagerKeys;
    vtkFilteringInformationKeyManagerKeys = nullptr;
    doomed->~vtkFilteringInformationKeyManagerKeysType();
    free(doomed);
  }

  // Release the lookup table; if no other manager still holds a
  // reference, it will be cleaned up here.
  vtkInformationKeyLookup::ReleaseCleanup();
}
VTK_ABI_NAMESPACE_END
