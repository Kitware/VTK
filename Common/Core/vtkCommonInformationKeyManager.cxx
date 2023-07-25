// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCommonInformationKeyManager.h"

#include "vtkInformationKey.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
// Subclass vector so we can directly call constructor.  This works
// around problems on Borland C++.
struct vtkCommonInformationKeyManagerKeysType : public std::vector<vtkInformationKey*>
{
  typedef std::vector<vtkInformationKey*> Superclass;
  typedef Superclass::iterator iterator;
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
  vtkCommonInformationKeyManagerKeys->push_back(key);
}

//------------------------------------------------------------------------------
void vtkCommonInformationKeyManager::ClassInitialize()
{
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
    // Delete information keys.
    for (vtkCommonInformationKeyManagerKeysType::iterator i =
           vtkCommonInformationKeyManagerKeys->begin();
         i != vtkCommonInformationKeyManagerKeys->end(); ++i)
    {
      vtkInformationKey* key = *i;
      delete key;
    }

    // Delete the singleton storing pointers to information keys.  See
    // ClassInitialize above for why this is a free instead of a
    // delete.
    vtkCommonInformationKeyManagerKeys->~vtkCommonInformationKeyManagerKeysType();
    free(vtkCommonInformationKeyManagerKeys);
    vtkCommonInformationKeyManagerKeys = nullptr;
  }
}
VTK_ABI_NAMESPACE_END
