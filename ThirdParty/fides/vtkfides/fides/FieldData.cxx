//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/FieldData.h>

namespace fides
{
namespace datamodel
{

FieldData::FieldData(const std::string& name,
                     const std::vector<vtkm::cont::UnknownArrayHandle>&& data)
  : Name(name)
  , Data(std::move(data))
{
}

std::string FieldData::GetName() const
{
  return this->Name;
}

const std::vector<vtkm::cont::UnknownArrayHandle>& FieldData::GetData() const
{
  return this->Data;
}

std::vector<vtkm::cont::UnknownArrayHandle>& FieldData::GetData()
{
  return this->Data;
}

}
}
