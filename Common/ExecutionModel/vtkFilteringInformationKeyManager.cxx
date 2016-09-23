/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFilteringInformationKeyManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFilteringInformationKeyManager.h"

#include "vtkInformationKey.h"

#include <vector>

// Subclass vector so we can directly call constructor.  This works
// around problems on Borland C++.
struct vtkFilteringInformationKeyManagerKeysType:
  public std::vector<vtkInformationKey*>
{
  typedef std::vector<vtkInformationKey*> Superclass;
  typedef Superclass::iterator iterator;
};

//----------------------------------------------------------------------------
// Must NOT be initialized.  Default initialization to zero is
// necessary.
static unsigned int vtkFilteringInformationKeyManagerCount;
static vtkFilteringInformationKeyManagerKeysType* vtkFilteringInformationKeyManagerKeys;

//----------------------------------------------------------------------------
vtkFilteringInformationKeyManager::vtkFilteringInformationKeyManager()
{
  if(++vtkFilteringInformationKeyManagerCount == 1)
  {
    vtkFilteringInformationKeyManager::ClassInitialize();
  }
}

//----------------------------------------------------------------------------
vtkFilteringInformationKeyManager::~vtkFilteringInformationKeyManager()
{
  if(--vtkFilteringInformationKeyManagerCount == 0)
  {
    vtkFilteringInformationKeyManager::ClassFinalize();
  }
}

//----------------------------------------------------------------------------
void vtkFilteringInformationKeyManager::Register(vtkInformationKey* key)
{
  // Register this instance for deletion by the singleton.
  vtkFilteringInformationKeyManagerKeys->push_back(key);
}

//----------------------------------------------------------------------------
void vtkFilteringInformationKeyManager::ClassInitialize()
{
  // Allocate the singleton storing pointers to information keys.
  // This must be a malloc/free pair instead of new/delete to work
  // around problems on MachO (Mac OS X) runtime systems that do lazy
  // symbol loading.  Calling operator new here causes static
  // initialization to occur in other translation units immediately,
  // which then may try to access the vector before it is set here.
  void* keys = malloc(sizeof(vtkFilteringInformationKeyManagerKeysType));
  vtkFilteringInformationKeyManagerKeys =
    new (keys) vtkFilteringInformationKeyManagerKeysType;
}

//----------------------------------------------------------------------------
void vtkFilteringInformationKeyManager::ClassFinalize()
{
  if(vtkFilteringInformationKeyManagerKeys)
  {
    // Delete information keys.
    for(vtkFilteringInformationKeyManagerKeysType::iterator i =
          vtkFilteringInformationKeyManagerKeys->begin();
        i != vtkFilteringInformationKeyManagerKeys->end(); ++i)
    {
      vtkInformationKey* key = *i;
      delete key;
    }

    // Delete the singleton storing pointers to information keys.  See
    // ClassInitialize above for why this is a free instead of a
    // delete.
    vtkFilteringInformationKeyManagerKeys->~vtkFilteringInformationKeyManagerKeysType();
    free(vtkFilteringInformationKeyManagerKeys);
    vtkFilteringInformationKeyManagerKeys = 0;
  }
}
