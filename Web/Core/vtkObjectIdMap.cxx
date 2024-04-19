// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkObjectIdMap.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <map>
#include <set>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
struct vtkObjectIdMap::vtkInternals
{
  std::map<vtkTypeUInt32, vtkSmartPointer<vtkObject>> Object;
  std::map<vtkSmartPointer<vtkObject>, vtkTypeUInt32> GlobalId;
  std::map<std::string, vtkWeakPointer<vtkObject>> ActiveObjects;
  vtkTypeUInt32 NextAvailableId;

  vtkInternals()
    : NextAvailableId(1)
  {
  }
};

vtkStandardNewMacro(vtkObjectIdMap);
//------------------------------------------------------------------------------
vtkObjectIdMap::vtkObjectIdMap()
  : Internals(new vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkObjectIdMap::~vtkObjectIdMap()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//------------------------------------------------------------------------------
void vtkObjectIdMap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkTypeUInt32 vtkObjectIdMap::GetGlobalId(vtkObject* obj)
{
  if (obj == nullptr)
  {
    return 0;
  }

  auto iter = this->Internals->GlobalId.find(obj);
  if (iter == this->Internals->GlobalId.end())
  {
    vtkTypeUInt32 globalId = this->Internals->NextAvailableId++;
    this->Internals->GlobalId[obj] = globalId;
    this->Internals->Object[globalId] = obj;
    return globalId;
  }
  return iter->second;
}

//------------------------------------------------------------------------------
vtkObject* vtkObjectIdMap::GetVTKObject(vtkTypeUInt32 globalId)
{
  auto iter = this->Internals->Object.find(globalId);
  if (iter == this->Internals->Object.end())
  {
    return nullptr;
  }
  return iter->second;
}

//------------------------------------------------------------------------------
vtkTypeUInt32 vtkObjectIdMap::SetActiveObject(const char* objectType, vtkObject* obj)
{
  if (objectType)
  {
    this->Internals->ActiveObjects[objectType] = obj;
    return this->GetGlobalId(obj);
  }
  return 0;
}

//------------------------------------------------------------------------------
vtkObject* vtkObjectIdMap::GetActiveObject(const char* objectType)
{
  if (objectType)
  {
    return this->Internals->ActiveObjects[objectType];
  }
  return nullptr;
}

//------------------------------------------------------------------------------
bool vtkObjectIdMap::FreeObject(vtkObject* obj)
{
  auto iter = this->Internals->GlobalId.find(obj);
  auto found = iter != this->Internals->GlobalId.end();
  if (found)
  {
    this->Internals->Object.erase(iter->second);
    this->Internals->GlobalId.erase(iter);
  }
  return found;
}

//------------------------------------------------------------------------------
bool vtkObjectIdMap::FreeObjectById(vtkTypeUInt32 id)
{
  auto iter = this->Internals->Object.find(id);
  auto found = iter != this->Internals->Object.end();
  if (found)
  {
    this->Internals->GlobalId.erase(iter->second);
    this->Internals->Object.erase(iter);
  }
  return found;
}
VTK_ABI_NAMESPACE_END
