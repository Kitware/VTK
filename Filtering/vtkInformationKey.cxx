/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationKey.h"

#include "vtkDebugLeaks.h"
#include "vtkInformation.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkInformationKey, "1.3");

class vtkInformationKeyToInformationFriendship
{
public:
  static void SetAsObjectBase(vtkInformation* info, vtkInformationKey* key,
                              vtkObjectBase* value)
    {
    info->SetAsObjectBase(key, value);
    }
  static vtkObjectBase* GetAsObjectBase(vtkInformation* info,
                                        vtkInformationKey* key)
    {
    return info->GetAsObjectBase(key);
    }
};

//----------------------------------------------------------------------------
// Purposely not initialized.  ClassInitialize will handle it.
vtkstd::vector<vtkInformationKey*>* vtkInformationKeyInstances;

//----------------------------------------------------------------------------
vtkInformationKey::vtkInformationKey(const char* name, const char* location)
{
  // Save the name and location.
  this->Name = name;
  this->Location = location;

  // Register this instance for deletion by the singleton.
  vtkInformationKeyInstances->push_back(this);
}

//----------------------------------------------------------------------------
vtkInformationKey::~vtkInformationKey()
{
  this->SetReferenceCount(0);
}

//----------------------------------------------------------------------------
void vtkInformationKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkInformationKey::Register(vtkObjectBase*)
{
}

//----------------------------------------------------------------------------
void vtkInformationKey::UnRegister(vtkObjectBase*)
{
}

//----------------------------------------------------------------------------
const char* vtkInformationKey::GetName()
{
  return this->Name;
}

//----------------------------------------------------------------------------
const char* vtkInformationKey::GetLocation()
{
  return this->Location;
}

//----------------------------------------------------------------------------
void vtkInformationKey::SetAsObjectBase(vtkInformation* info,
                                        vtkObjectBase* value)
{
  vtkInformationKeyToInformationFriendship::SetAsObjectBase(info, this, value);
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkInformationKey::GetAsObjectBase(vtkInformation* info)
{
  return vtkInformationKeyToInformationFriendship::GetAsObjectBase(info, this);
}

//----------------------------------------------------------------------------
void vtkInformationKey::Remove(vtkInformation* info)
{
  this->SetAsObjectBase(info, 0);
}

//----------------------------------------------------------------------------
void vtkInformationKey::Report(vtkInformation*, vtkGarbageCollector*)
{
  // Report nothing by default.
}

//----------------------------------------------------------------------------
#ifdef VTK_DEBUG_LEAKS
void vtkInformationKey::ConstructClass(const char* name)
{
  vtkDebugLeaks::ConstructClass(name);
}
#else
void vtkInformationKey::ConstructClass(const char*)
{
}
#endif

//----------------------------------------------------------------------------
void vtkInformationKey::ClassInitialize()
{
  // Allocate the singleton storing pointers to all information keys.
  vtkInformationKeyInstances = new vtkstd::vector<vtkInformationKey*>;
}

//----------------------------------------------------------------------------
void vtkInformationKey::ClassFinalize()
{
  if(vtkInformationKeyInstances)
    {
    // Delete all information keys.
    for(vtkstd::vector<vtkInformationKey*>::iterator i =
          vtkInformationKeyInstances->begin();
        i != vtkInformationKeyInstances->end(); ++i)
      {
      vtkInformationKey* key = *i;
      delete key;
      }

    // Delete the singleton storing pointers to all information keys.
    delete vtkInformationKeyInstances;
    vtkInformationKeyInstances = 0;
    }
}
