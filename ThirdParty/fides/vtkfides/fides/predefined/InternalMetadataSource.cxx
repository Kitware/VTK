//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/predefined/InternalMetadataSource.h>

namespace fides
{
namespace predefined
{

namespace detail
{

std::string ReadSingleValue(std::shared_ptr<fides::io::DataSource> source,
  const std::string& attrName)
{
  if (source->GetAttributeType(attrName) != "string")
  {
    throw std::runtime_error("Attribute " + attrName + " should have type string");
  }
  auto attr = source->ReadAttribute<std::string>(attrName);
  if (attr.size() != 1)
  {
    throw std::runtime_error("Fides was not able to read " + attrName +
      " from file " + source->FileName);
  }
  return attr[0];
}

};

InternalMetadataSource::InternalMetadataSource(const std::string& filename)
{
  this->Source.reset(new fides::io::DataSource());
  this->Source->Mode = fides::io::FileNameMode::Relative;
  this->Source->FileName = filename;
  this->Source->OpenSource(filename);
}

InternalMetadataSource::~InternalMetadataSource() = default;

std::string InternalMetadataSource::GetDataModelName(const std::string& attrName)
{
  return detail::ReadSingleValue(this->Source, attrName);
}

DataModelTypes InternalMetadataSource::GetDataModelType(const std::string& attrName)
{
  auto model = detail::ReadSingleValue(this->Source, attrName);
  return ConvertDataModelToEnum(model);
}

std::string InternalMetadataSource::GetDataModelCellType(const std::string& attrName)
{
  return detail::ReadSingleValue(this->Source, attrName);
}

std::string InternalMetadataSource::GetAttributeType(const std::string& attrName)
{
  return this->Source->GetAttributeType(attrName);
}

}
}
