//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/FieldDataManager.h>

namespace fides
{
namespace datamodel
{

void FieldDataManager::AddField(const std::string& name,
  FieldData array)
{
  auto rc = this->Data.insert(std::make_pair(name, array));
  if (!rc.second)
  {
    throw std::runtime_error("Field " + name + " already exists!");
  }
}

bool FieldDataManager::HasField(const std::string& name)
{
  return this->Data.find(name) != this->Data.end();
}

FieldData& FieldDataManager::GetField(const std::string& name)
{
  if (this->HasField(name))
  {
    return this->Data[name];
  }
  else
  {
    throw std::runtime_error("Field " + name + " not found");
  }
}

const std::unordered_map<std::string, FieldData>& FieldDataManager::GetAllFields()
{
  return this->Data;
}

void FieldDataManager::Clear()
{
  this->Data.clear();
}

}
}
