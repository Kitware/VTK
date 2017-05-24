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
#include "vtkInformationKeyLookup.h"

#include "vtkDebugLeaks.h"
#include "vtkInformation.h"


class vtkInformationKeyToInformationFriendship
{
public:
  static void SetAsObjectBase(vtkInformation* info, vtkInformationKey* key,
                              vtkObjectBase* value)
  {
    info->SetAsObjectBase(key, value);
  }
  static const vtkObjectBase* GetAsObjectBase(const vtkInformation* info,
                                        const vtkInformationKey* key)
  {
    return info->GetAsObjectBase(key);
  }
  static vtkObjectBase* GetAsObjectBase(vtkInformation* info,
                                        vtkInformationKey* key)
  {
    return info->GetAsObjectBase(key);
  }
  static void ReportAsObjectBase(vtkInformation* info, vtkInformationKey* key,
                                 vtkGarbageCollector* collector)
  {
    info->ReportAsObjectBase(key, collector);
  }
};

//----------------------------------------------------------------------------
vtkInformationKey::vtkInformationKey(const char* name, const char* location)
{
  // Save the name and location.
  this->Name = 0;
  this->SetName(name);

  this->Location = 0;
  this->SetLocation(location);

  vtkInformationKeyLookup::RegisterKey(this, name, location);
}

//----------------------------------------------------------------------------
vtkInformationKey::~vtkInformationKey()
{
  this->SetReferenceCount(0);
  this->SetName(0);
  this->SetLocation(0);
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
const vtkObjectBase* vtkInformationKey::GetAsObjectBase(
  vtkInformation* info) const
{
  return vtkInformationKeyToInformationFriendship::GetAsObjectBase(info, this);
}

//----------------------------------------------------------------------------
int vtkInformationKey::Has(vtkInformation* info)
{
  return this->GetAsObjectBase(info)?1:0;
}

//----------------------------------------------------------------------------
void vtkInformationKey::Remove(vtkInformation* info)
{
  this->SetAsObjectBase(info, nullptr);
}

//----------------------------------------------------------------------------
void vtkInformationKey::Report(vtkInformation*, vtkGarbageCollector*)
{
  // Report nothing by default.
}

//----------------------------------------------------------------------------
void vtkInformationKey::Print(vtkInformation* info)
{
  this->Print(cout, info);
}

//----------------------------------------------------------------------------
void vtkInformationKey::Print(ostream& os, vtkInformation* info)
{
  // Just print the value type and pointer by default.
  if(vtkObjectBase* value = this->GetAsObjectBase(info))
  {
    os << value->GetClassName() << "(" << value << ")";
  }
}

//----------------------------------------------------------------------------
void vtkInformationKey::ReportAsObjectBase(vtkInformation* info,
                                           vtkGarbageCollector* collector)
{
  vtkInformationKeyToInformationFriendship::ReportAsObjectBase(info, this,
                                                               collector);
}

//----------------------------------------------------------------------------
void vtkInformationKey::ConstructClass(const char*)
{
}
