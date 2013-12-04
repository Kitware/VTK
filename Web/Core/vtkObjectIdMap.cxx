/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObjectIdMap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObjectIdMap.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <map>
#include <string>
#include <set>

namespace
{
struct ObjectId
{
  ObjectId()
  {
    this->GlobalId = 0;
    this->Object = NULL;
  }

  ObjectId(vtkTypeUInt32 id)
  {
    this->GlobalId = id;
    this->Object = NULL;
  }

  ObjectId(vtkObject* obj, vtkTypeUInt32 id = 0)
  {
    this->GlobalId = id;
    this->Object = obj;
  }

  ObjectId(const ObjectId& other)
  {
    this->GlobalId = other.GlobalId;
    this->Object = other.Object;
  }

  ObjectId& operator=(const ObjectId &other)
  {
    if (this == &other)
      {
      // Same object?
      return *this;
      }

    this->GlobalId = other.GlobalId;
    this->Object = other.Object;
    return *this;
  }

  ObjectId& operator=(vtkTypeUInt32 id)
  {
    this->GlobalId = id;
    this->Object = NULL;
    return *this;
  }

  ObjectId& operator=(vtkObject* obj)
  {
    this->GlobalId = 0;
    this->Object = obj;
    return *this;
  }

  bool operator==(const ObjectId &other) const
  {
    if (this == &other)
      {
      // Same object?
      return true;
      }

    if( this->GlobalId != 0 && other.GlobalId != 0
        && this->GlobalId != other.GlobalId)
      {
      return false;
      }

    if( this->Object.GetPointer() != NULL && other.Object.GetPointer() != NULL
        && this->Object.GetPointer() != other.Object.GetPointer())
      {
      return false;
      }

    return true;
  }

   bool operator!=(const ObjectId &other) const
   {
     return !(*this == other);
   }

   bool operator<(const ObjectId &other) const
   {
     if (this == &other)
       {
       // Same object?
       return false;
       }

     if( this->GlobalId != 0 && other.GlobalId != 0
         && this->GlobalId != other.GlobalId)
       {
       return (this->GlobalId < other.GlobalId);
       }

     if( this->Object.GetPointer() != NULL && other.Object.GetPointer() != NULL
         && this->Object.GetPointer() != other.Object.GetPointer())
       {
       return (this->Object.GetPointer() < other.Object.GetPointer());
       }

     return false;
   }

  vtkSmartPointer<vtkObject> Object;
  vtkTypeUInt32 GlobalId;
};
}

struct vtkObjectIdMap::vtkInternals
{
  std::set<ObjectId> RegisteredObjects;
  std::map<std::string, vtkWeakPointer<vtkObject> > ActiveObjects;
  vtkTypeUInt32 NextAvailableId;

  vtkInternals() : NextAvailableId(1)
  {
  }

};

vtkStandardNewMacro(vtkObjectIdMap);
//----------------------------------------------------------------------------
vtkObjectIdMap::vtkObjectIdMap() :
  Internals(new vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkObjectIdMap::~vtkObjectIdMap()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkObjectIdMap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkObjectIdMap::GetGlobalId(vtkObject* obj)
{
  if(obj == NULL)
    {
    return 0;
    }

  ObjectId key(obj, 0);
  std::set<ObjectId>::iterator iter = this->Internals->RegisteredObjects.find(key);
  if(iter == this->Internals->RegisteredObjects.end())
    {
    key.GlobalId = this->Internals->NextAvailableId++;
    this->Internals->RegisteredObjects.insert(key);
    return key.GlobalId;
    }
  return iter->GlobalId;
}

//----------------------------------------------------------------------------
vtkObject* vtkObjectIdMap::GetVTKObject(vtkTypeUInt32 globalId)
{
  ObjectId key(globalId);
  std::set<ObjectId>::iterator iter = this->Internals->RegisteredObjects.find(key);
  if(iter == this->Internals->RegisteredObjects.end())
    {
    return NULL;
    }
  return iter->Object.GetPointer();
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkObjectIdMap::SetActiveObject(const char* objectType, vtkObject* obj)
{
  if(objectType)
    {
    this->Internals->ActiveObjects[objectType] = obj;
    return this->GetGlobalId(obj);
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkObject* vtkObjectIdMap::GetActiveObject(const char* objectType)
{
  if(objectType)
    {
    return this->Internals->ActiveObjects[objectType].GetPointer();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkObjectIdMap::FreeObject(vtkObject* obj)
{
  this->Internals->RegisteredObjects.erase(obj);
}
