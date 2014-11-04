/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCommonDataModelModule.h" // needed for proper visibility defines for the header.

#include "vtkInformation.h"

#include "vtkInformationDataObjectKey.h"
#include "vtkInformationKeyVectorKey.h"

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationDataObjectKey* key, int deep)
{
  if (!deep)
    {
    key->ShallowCopy(from, this);
    }
  else
    {
    key->DeepCopy(from, this);
    }
}

//----------------------------------------------------------------------------
#define VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(name, type)                  \
  void vtkInformation::Set(vtkInformation##name##Key* key, type value)      \
    {                                                                       \
    key->Set(this, value);                                                  \
    }                                                                       \
  void vtkInformation::Remove(vtkInformation##name##Key* key)               \
    {                                                                       \
    key->vtkInformation##name##Key::Remove(this);                           \
    }                                                                       \
  type vtkInformation::Get(vtkInformation##name##Key* key)                  \
    {                                                                       \
    return key->Get(this);                                                  \
    }                                                                       \
  int vtkInformation::Has(vtkInformation##name##Key* key)                   \
    {                                                                       \
    return key->vtkInformation##name##Key::Has(this);                       \
    }
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(DataObject, vtkDataObject*);
#undef VTK_INFORMATION_DEFINE_SCALAR_PROPERTY

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationDataObjectKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationDataObjectKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationDataObjectKey* key)
{
  return key;
}
