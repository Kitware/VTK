// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInformationKey.h"
#include "vtkInformationKeyLookup.h"

#include "vtkDebugLeaks.h"
#include "vtkInformation.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationKeyToInformationFriendship
{
public:
  static void SetAsObjectBase(vtkInformation* info, vtkInformationKey* key, vtkObjectBase* value)
  {
    info->SetAsObjectBase(key, value);
  }
  static const vtkObjectBase* GetAsObjectBase(
    const vtkInformation* info, const vtkInformationKey* key)
  {
    return info->GetAsObjectBase(key);
  }
  static vtkObjectBase* GetAsObjectBase(vtkInformation* info, vtkInformationKey* key)
  {
    return info->GetAsObjectBase(key);
  }
  static void ReportAsObjectBase(
    vtkInformation* info, vtkInformationKey* key, vtkGarbageCollector* collector)
  {
    info->ReportAsObjectBase(key, collector);
  }
};

//------------------------------------------------------------------------------
vtkInformationKey::vtkInformationKey(const char* name, const char* location)
{
  // Save the name and location.
  this->Name = nullptr;
  this->SetName(name);

  this->Location = nullptr;
  this->SetLocation(location);

  vtkInformationKeyLookup::RegisterKey(this, name, location);
}

//------------------------------------------------------------------------------
vtkInformationKey::~vtkInformationKey()
{
  // Avoid warnings from `vtkObjectBase` destructor.
  this->ClearReferenceCounts();
  this->SetName(nullptr);
  this->SetLocation(nullptr);
}

//------------------------------------------------------------------------------
void vtkInformationKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
const char* vtkInformationKey::GetName() VTK_FUTURE_CONST
{
  return this->Name;
}

//------------------------------------------------------------------------------
const char* vtkInformationKey::GetLocation() VTK_FUTURE_CONST
{
  return this->Location;
}

//------------------------------------------------------------------------------
void vtkInformationKey::SetAsObjectBase(vtkInformation* info, vtkObjectBase* value)
{
  vtkInformationKeyToInformationFriendship::SetAsObjectBase(info, this, value);
}

//------------------------------------------------------------------------------
vtkObjectBase* vtkInformationKey::GetAsObjectBase(vtkInformation* info)
{
  return vtkInformationKeyToInformationFriendship::GetAsObjectBase(info, this);
}

//------------------------------------------------------------------------------
const vtkObjectBase* vtkInformationKey::GetAsObjectBase(VTK_FUTURE_CONST vtkInformation* info) const
{
  return vtkInformationKeyToInformationFriendship::GetAsObjectBase(info, this);
}

//------------------------------------------------------------------------------
int vtkInformationKey::Has(VTK_FUTURE_CONST vtkInformation* info) VTK_FUTURE_CONST
{
  return this->GetAsObjectBase(info) ? 1 : 0;
}

//------------------------------------------------------------------------------
void vtkInformationKey::Remove(vtkInformation* info)
{
  this->SetAsObjectBase(info, nullptr);
}

//------------------------------------------------------------------------------
void vtkInformationKey::Report(vtkInformation*, vtkGarbageCollector*)
{
  // Report nothing by default.
}

//------------------------------------------------------------------------------
void vtkInformationKey::Print(vtkInformation* info)
{
  this->Print(cout, info);
}

//------------------------------------------------------------------------------
void vtkInformationKey::Print(ostream& os, vtkInformation* info)
{
  // Just print the value type and pointer by default.
  if (vtkObjectBase* value = this->GetAsObjectBase(info))
  {
    os << value->GetClassName() << "(" << value << ")";
  }
}

//------------------------------------------------------------------------------
void vtkInformationKey::ReportAsObjectBase(vtkInformation* info, vtkGarbageCollector* collector)
{
  vtkInformationKeyToInformationFriendship::ReportAsObjectBase(info, this, collector);
}

//------------------------------------------------------------------------------
void vtkInformationKey::ConstructClass(const char*) {}
VTK_ABI_NAMESPACE_END
