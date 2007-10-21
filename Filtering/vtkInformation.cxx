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
#include "vtkInformation.h" // Must be last include

#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationExecutivePortVectorKey.h"
#include "vtkInformationKeyVectorKey.h"

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, 
                               vtkInformationExecutivePortKey* key, int)
{
  key->ShallowCopy(from, this);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationExecutivePortKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationExecutivePortKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Set(vtkInformationExecutivePortKey* key,
                         vtkExecutive* executive, int port)
{
  key->Set(this, executive, port);
}

//----------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationExecutivePortKey* key)
{
  key->Remove(this);
}

//----------------------------------------------------------------------------
vtkExecutive* vtkInformation::GetExecutive(vtkInformationExecutivePortKey* key)
{
  return key->GetExecutive(this);
}

//----------------------------------------------------------------------------
int vtkInformation::GetPort(vtkInformationExecutivePortKey* key)
{
  return key->GetPort(this);
}

//----------------------------------------------------------------------------
void vtkInformation::Get(vtkInformationExecutivePortKey* key,
                         vtkExecutive*& executive, int &port)
{
  key->Get(this,executive,port);
}

//----------------------------------------------------------------------------
int vtkInformation::Has(vtkInformationExecutivePortKey* key)
{
  return key->Has(this);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationExecutivePortVectorKey* key,
                            vtkExecutive* executive, int port)
{
  key->Append(this, executive, port);
}

//----------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationExecutivePortVectorKey* key,
                            vtkExecutive* executive, int port)
{
  key->Remove(this, executive, port);
}

//----------------------------------------------------------------------------
void vtkInformation::Set(vtkInformationExecutivePortVectorKey* key,
                         vtkExecutive** executives, int* ports, int length)
{
  key->Set(this, executives, ports, length);
}

//----------------------------------------------------------------------------
vtkExecutive**
vtkInformation::GetExecutives(vtkInformationExecutivePortVectorKey* key)
{
  return key->GetExecutives(this);
}

//----------------------------------------------------------------------------
int*
vtkInformation::GetPorts(vtkInformationExecutivePortVectorKey* key)
{
  return key->GetPorts(this);
}

//----------------------------------------------------------------------------
void vtkInformation::Get(vtkInformationExecutivePortVectorKey* key,
                         vtkExecutive** executives, int* ports)
{
  key->Get(this, executives, ports);
}

//----------------------------------------------------------------------------
int vtkInformation::Length(vtkInformationExecutivePortVectorKey* key)
{
  return key->Length(this);
}

//----------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationExecutivePortVectorKey* key)
{
  key->Remove(this);
}

//----------------------------------------------------------------------------
int vtkInformation::Has(vtkInformationExecutivePortVectorKey* key)
{
  return key->Has(this);
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationExecutivePortKey* key)
{
  return key;
}


