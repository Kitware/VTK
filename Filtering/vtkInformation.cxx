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
#include "vtkInformation.h"

#ifndef VTK_LEGACY_REMOVE

#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationExecutivePortVectorKey.h"
#include "vtkInformationKeyVectorKey.h"

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, 
                               vtkInformationExecutivePortKey* key, int)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::CopyEntry, "VTK 5.2",
    vtkInformationExecutivePortKey::ShallowCopy);
  key->ShallowCopy(from, this);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationExecutivePortKey* value)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::Append, "VTK 5.2",
    vtkInformationExecutivePortKey::Append);
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationExecutivePortKey* value)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::AppendUnique, "VTK 5.2",
    vtkInformationExecutivePortKey::AppendUnique);
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Set(vtkInformationExecutivePortKey* key,
                         vtkExecutive* executive, int port)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::Set, "VTK 5.2",
    vtkInformationExecutivePortKey::Set);
  key->Set(this, executive, port);
}

//----------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationExecutivePortKey* key)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::Remove, "VTK 5.2",
    vtkInformationExecutivePortKey::Remove);
  key->Remove(this);
}

//----------------------------------------------------------------------------
vtkExecutive* vtkInformation::GetExecutive(vtkInformationExecutivePortKey* key)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::GetExecutive, "VTK 5.2",
    vtkInformationExecutivePortKey::GetExecutive);
  return key->GetExecutive(this);
}

//----------------------------------------------------------------------------
int vtkInformation::GetPort(vtkInformationExecutivePortKey* key)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::GetPort, "VTK 5.2",
    vtkInformationExecutivePortKey::GetPort);
  return key->GetPort(this);
}

//----------------------------------------------------------------------------
void vtkInformation::Get(vtkInformationExecutivePortKey* key,
                         vtkExecutive*& executive, int &port)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::Get, "VTK 5.2",
    vtkInformationExecutivePortKey::Get);
  key->Get(this,executive,port);
}

//----------------------------------------------------------------------------
int vtkInformation::Has(vtkInformationExecutivePortKey* key)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::Has, "VTK 5.2",
    vtkInformationExecutivePortKey::Has);
  return key->Has(this);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationExecutivePortVectorKey* key,
                            vtkExecutive* executive, int port)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::Append, "VTK 5.2",
    vtkInformationExecutivePortVectorKey::Append);
  key->Append(this, executive, port);
}

//----------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationExecutivePortVectorKey* key,
                            vtkExecutive* executive, int port)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::Remove, "VTK 5.2",
    vtkInformationExecutivePortVectorKey::Remove);
  key->Remove(this, executive, port);
}

//----------------------------------------------------------------------------
void vtkInformation::Set(vtkInformationExecutivePortVectorKey* key,
                         vtkExecutive** executives, int* ports, int length)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::Set, "VTK 5.2",
    vtkInformationExecutivePortVectorKey::Set);
  key->Set(this, executives, ports, length);
}

//----------------------------------------------------------------------------
vtkExecutive**
vtkInformation::GetExecutives(vtkInformationExecutivePortVectorKey* key)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::GetExecutives, "VTK 5.2",
    vtkInformationExecutivePortVectorKey::GetExecutives);
  return key->GetExecutives(this);
}

//----------------------------------------------------------------------------
int*
vtkInformation::GetPorts(vtkInformationExecutivePortVectorKey* key)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::GetPorts, "VTK 5.2",
    vtkInformationExecutivePortVectorKey::GetPorts);
  return key->GetPorts(this);
}

//----------------------------------------------------------------------------
void vtkInformation::Get(vtkInformationExecutivePortVectorKey* key,
                         vtkExecutive** executives, int* ports)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::Get, "VTK 5.2",
    vtkInformationExecutivePortVectorKey::Get);
  key->Get(this, executives, ports);
}

//----------------------------------------------------------------------------
int vtkInformation::Length(vtkInformationExecutivePortVectorKey* key)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::Length, "VTK 5.2",
    vtkInformationExecutivePortVectorKey::Length);
  return key->Length(this);
}

//----------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationExecutivePortVectorKey* key)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::Remove, "VTK 5.2",
    vtkInformationExecutivePortVectorKey::Remove);
  key->Remove(this);
}

//----------------------------------------------------------------------------
int vtkInformation::Has(vtkInformationExecutivePortVectorKey* key)
{
  VTK_LEGACY_REPLACED_BODY(vtkInformation::Has, "VTK 5.2",
    vtkInformationExecutivePortVectorKey::Has);
  return key->Has(this);
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationExecutivePortKey* key)
{
  VTK_LEGACY_BODY(vtkInformation::GetKey, "VTK 5.2");
  return key;
}

#endif // VTK_LEGACY_REMOVE

