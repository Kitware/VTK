//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/Field.h>

namespace fides
{
namespace datamodel
{

void Field::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  this->Array.reset();
  if (json.HasMember("name") && json["name"].IsString())
  {
    this->Name = json["name"].GetString();
  }
  else if (json.HasMember("variable_list_attribute_name") &&
           json["variable_list_attribute_name"].IsString())
  {
    this->VariableAttributeName = json["variable_list_attribute_name"].GetString();
    // Name is actually needed for being able to keep track of these in the dataset reader
    this->Name = this->VariableAttributeName;
    this->WildcardField = true;
  }
  else
  {
    throw std::runtime_error(this->ObjectName + " must provide a valid name.");
  }

  if (json.HasMember("association") && json["association"].IsString())
  {
    const std::string& assoc = json["association"].GetString();
    if (assoc == "points")
    {
      this->Association = vtkm::cont::Field::Association::Points;
    }
    else if (assoc == "cell_set")
    {
      this->Association = vtkm::cont::Field::Association::Cells;
    }
    else if (assoc == "field_data")
    {
      this->Association = vtkm::cont::Field::Association::WholeDataSet;
    }
    else
    {
      throw std::runtime_error(this->ObjectName + " provided unknown association: " + assoc);
    }
  }
  else if (json.HasMember("variable_association_attribute_name") &&
           json["variable_association_attribute_name"].IsString())
  {
    this->AssociationAttributeName = json["variable_association_attribute_name"].GetString();
  }
  else
  {
    throw std::runtime_error(this->ObjectName +
                             " must provide a valid association (points or cell_set).");
  }

  if (json.HasMember("variable_vector_attribute_name") &&
      json["variable_vector_attribute_name"].IsString())
  {
    this->VectorAttributeName = json["variable_vector_attribute_name"].GetString();
  }

  if (json.HasMember("variable_sources_attribute_name") &&
      json["variable_sources_attribute_name"].IsString())
  {
    this->SourcesAttributeName = json["variable_sources_attribute_name"].GetString();
  }

  if (json.HasMember("variable_arrays_attribute_name") &&
      json["variable_arrays_attribute_name"].IsString())
  {
    this->ArrayTypesAttributeName = json["variable_arrays_attribute_name"].GetString();
  }

  if (!json.HasMember("array") || !json["array"].IsObject())
  {
    throw std::runtime_error(this->ObjectName + " must provide an array object.");
  }
  this->Array = std::make_shared<fides::datamodel::Array>();
  this->Array->ObjectName = "array";
  if (this->WildcardField)
  {
    this->Array->CreatePlaceholder(json["array"], sources);
  }
  else
  {
    this->Array->ProcessJSON(json["array"], sources);
  }
}

void Field::ProcessExpandedField(const std::string& name,
                                 const std::string& assoc,
                                 const rapidjson::Value& json,
                                 DataSourcesType& sources)
{
  this->Name = name;
  this->WildcardField = false; // no longer a wildcard field now
  if (assoc == "points")
  {
    this->Association = vtkm::cont::Field::Association::Points;
  }
  else if (assoc == "cell_set")
  {
    this->Association = vtkm::cont::Field::Association::Cells;
  }
  else if (assoc == "field_data")
  {
    this->Association = vtkm::cont::Field::Association::WholeDataSet;
  }
  else
  {
    throw std::runtime_error(this->ObjectName + " provided unknown association: " + assoc);
  }
  this->Array.reset();
  this->Array = std::make_shared<fides::datamodel::Array>();
  this->Array->ObjectName = "array";
  this->Array->ProcessJSON(json["array"], sources);
}

std::vector<vtkm::cont::Field> Field::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  std::vector<vtkm::cont::UnknownArrayHandle> arrays =
    this->Array->Read(paths, sources, selections);
  std::vector<vtkm::cont::Field> fields;
  size_t nFields = arrays.size();
  fields.reserve(nFields);
  for (size_t i = 0; i < nFields; i++)
  {
    vtkm::cont::Field fld(this->Name, this->Association, arrays[i]);
    fields.push_back(fld);
  }

  return fields;
}

void Field::PostRead(std::vector<vtkm::cont::DataSet>& partitions,
                     const fides::metadata::MetaData& selections)
{
  this->Array->PostRead(partitions, selections);
}

FIDES_DEPRECATED_SUPPRESS_BEGIN
FieldData Field::ReadFieldData(const std::unordered_map<std::string, std::string>& paths,
                               DataSourcesType& sources,
                               const fides::metadata::MetaData& selections)
{
  std::vector<vtkm::cont::UnknownArrayHandle> arrays =
    this->Array->Read(paths, sources, selections);
  return FieldData(this->Name, std::move(arrays));
}
FIDES_DEPRECATED_SUPPRESS_END

Field::WildcardFieldInfo Field::GetWildcardFieldLists(
  std::shared_ptr<predefined::InternalMetadataSource> source)
{
  if (!this->WildcardField)
  {
    throw std::runtime_error("GetWildcardFieldLists() should not be called on a normal field");
  }

  WildcardFieldInfo fieldInfo;
  fieldInfo.Names = source->GetAttribute<std::string>(this->VariableAttributeName);
  if (fieldInfo.Names.empty())
  {
    throw std::runtime_error("Fides was not able to read std::string attribute " +
                             this->VariableAttributeName);
  }

  fieldInfo.Associations = source->GetAttribute<std::string>(this->AssociationAttributeName);
  if (fieldInfo.Associations.empty())
  {
    throw std::runtime_error("Fides was not able to read std::string attribute " +
                             this->AssociationAttributeName);
  }

  fieldInfo.IsVector = source->GetAttribute<std::string>(this->VectorAttributeName);
  fieldInfo.Sources = source->GetAttribute<std::string>(this->SourcesAttributeName);
  fieldInfo.ArrayTypes = source->GetAttribute<std::string>(this->ArrayTypesAttributeName);

  if (fieldInfo.Names.size() != fieldInfo.Associations.size())
  {
    throw std::runtime_error(
      "The arrays read for Field Names and Associations should be the same size");
  }
  if (!fieldInfo.IsVector.empty() && fieldInfo.IsVector.size() != fieldInfo.Names.size())
  {
    throw std::runtime_error("If the array read for Field's is vector is not empty, it should be"
                             " the same size as the Names array");
  }
  if (!fieldInfo.Sources.empty() && fieldInfo.Sources.size() != fieldInfo.Names.size())
  {
    throw std::runtime_error("If the arrays read for Field data sources is not empty, it should be"
                             " the same size as the Names array");
  }
  if (!fieldInfo.ArrayTypes.empty() && fieldInfo.ArrayTypes.size() != fieldInfo.Names.size())
  {
    throw std::runtime_error("If the arrays read for Field array types is not empty, it should be"
                             " the same size as the Names array");
  }
  return fieldInfo;
}

}
}
