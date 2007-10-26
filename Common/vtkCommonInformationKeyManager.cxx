/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommonInformationKeyManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCommonInformationKeyManager.h"

#include "vtkInformationKey.h"

#include <vtkstd/vector>

// Subclass vector so we can directly call constructor.  This works
// around problems on Borland C++.
struct vtkCommonInformationKeyManagerKeysType:
  public vtkstd::vector<vtkInformationKey*>
{
  typedef vtkstd::vector<vtkInformationKey*> Superclass;
  typedef Superclass::iterator iterator;
};

//----------------------------------------------------------------------------
// Must NOT be initialized.  Default initialization to zero is
// necessary.
static unsigned int vtkCommonInformationKeyManagerCount;
static vtkCommonInformationKeyManagerKeysType* vtkCommonInformationKeyManagerKeys;

//----------------------------------------------------------------------------
vtkCommonInformationKeyManager::vtkCommonInformationKeyManager()
{
  if(++vtkCommonInformationKeyManagerCount == 1)
    {
    vtkCommonInformationKeyManager::ClassInitialize();
    }
}

//----------------------------------------------------------------------------
vtkCommonInformationKeyManager::~vtkCommonInformationKeyManager()
{
  if(--vtkCommonInformationKeyManagerCount == 0)
    {
    vtkCommonInformationKeyManager::ClassFinalize();
    }
}

//----------------------------------------------------------------------------
void vtkCommonInformationKeyManager::Register(vtkInformationKey* key)
{
  // Register this instance for deletion by the singleton.
  vtkCommonInformationKeyManagerKeys->push_back(key);
}

//----------------------------------------------------------------------------
void vtkCommonInformationKeyManager::ClassInitialize()
{
  // Allocate the singleton storing pointers to information keys.
  // This must be a malloc/free pair instead of new/delete to work
  // around problems on MachO (Mac OS X) runtime systems that do lazy
  // symbol loading.  Calling operator new here causes static
  // initialization to occur in other translation units immediately,
  // which then may try to access the vector before it is set here.
  void* keys = malloc(sizeof(vtkCommonInformationKeyManagerKeysType));
  vtkCommonInformationKeyManagerKeys =
    new (keys) vtkCommonInformationKeyManagerKeysType;
}

//----------------------------------------------------------------------------
void vtkCommonInformationKeyManager::ClassFinalize()
{
  if(vtkCommonInformationKeyManagerKeys)
    {
    // Delete information keys.
    for(vtkCommonInformationKeyManagerKeysType::iterator i =
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
    vtkCommonInformationKeyManagerKeys = 0;
    }
}
