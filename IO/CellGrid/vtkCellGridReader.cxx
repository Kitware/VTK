// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGridReader.h"

#include "vtkCellAttribute.h"
#include "vtkCellGridIOQuery.h"
#include "vtkCellMetadata.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIOCellGrid.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"

#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)

#include <array>
#include <sstream>

namespace
{

int ArrayTypeToEnum(const std::string& arrayType)
{
  int result = -1;
  if (arrayType == "int")
  {
    result = VTK_INT;
  }
  else if (arrayType == "vtktypeint64")
  {
    result = VTK_TYPE_INT64;
  }
  else if (arrayType == "double")
  {
    result = VTK_DOUBLE;
  }
  else if (arrayType == "float")
  {
    result = VTK_FLOAT;
  }
  return result;
}

template <typename T>
void AppendArrayData(T* data, nlohmann::json& values)
{
  auto valueVector = values.get<std::vector<T>>();
  vtkIdType ii = 0;
  for (const auto& value : valueVector)
  {
    data[ii] = value;
    ++ii;
  }
}

} // anonymous namespace

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCellGridReader);

vtkCellGridReader::vtkCellGridReader()
{
  this->SetNumberOfInputPorts(0);
  vtkFiltersCellGrid::RegisterCellsAndResponders();
  vtkIOCellGrid::RegisterCellsAndResponders();
}

vtkCellGridReader::~vtkCellGridReader()
{
  this->SetFileName(nullptr);
}

void vtkCellGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: \"" << (this->FileName ? this->FileName : "(none)") << "\"\n";
}

int vtkCellGridReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
  {
    return 0;
  }

  // Read file metadata
  // Make sure we have a file to read.
  if (!this->FileName)
  {
    vtkErrorMacro("A FileName must be specified.");
    return 0;
  }

  std::string fileNameAsString(this->FileName);

  if (fileNameAsString.find('\\') != std::string::npos)
  {
    vtksys::SystemTools::ConvertToUnixSlashes(fileNameAsString);
  }

  if (!vtksys::SystemTools::FileIsFullPath(fileNameAsString))
  {
    fileNameAsString = vtksys::SystemTools::CollapseFullPath(fileNameAsString);
  }

  if (this->FileName != fileNameAsString)
  {
    this->SetFileName(fileNameAsString.c_str());
  }

  // TODO: Load metadata if not done previously
  //       Add this info to the output vtkInformation
  // vtkInformation* info = outputVector->GetInformationObject(0);
  // info->Append(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), i * period);
  // info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange.data(), 2);
  return 1;
}

int vtkCellGridReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  // Get the output
  vtkCellGrid* output = vtkCellGrid::GetData(outputVector);

  // Make sure we have a file to read.
  if (!this->FileName)
  {
    vtkErrorMacro("A FileName must be specified.");
    return 0;
  }

  // Check the file's validity.
  std::ifstream file(this->FileName);
  if (!file.good())
  {
    vtkErrorMacro("Cannot read file \"" << this->FileName << "\".");
    return 0;
  }

  // Read the file into nlohmann json.
  nlohmann::json jj;
  try
  {
    jj = nlohmann::json::parse(file);
  }
  catch (...)
  {
    vtkErrorMacro("Cannot parse file \"" << this->FileName << "\".");
    return 0;
  }

  auto jtype = jj.find("data-type");
  if (jtype == jj.end() || jtype->get<std::string>() != "cell-grid")
  {
    vtkErrorMacro("Data type is missing or incorrect.");
    return 0;
  }

  auto jArrayGroup = jj.find("arrays");
  if (jArrayGroup == jj.end() || !jArrayGroup->is_object())
  {
    vtkErrorMacro("Missing arrays section.");
    return 0;
  }

  auto jAttributes = jj.find("attributes");
  if (jAttributes == jj.end() || !jAttributes->is_array())
  {
    vtkErrorMacro("Missing attributes section.");
    return 0;
  }

  auto jCellTypes = jj.find("cell-types");
  if (jCellTypes == jj.end() || !jCellTypes->is_array())
  {
    vtkErrorMacro("Missing cell-types section.");
    return 0;
  }

  bool skipVersionChecks = false;
  auto jSchemaName = jj.find("schema-name");
  auto jSchemaVersion = jj.find("schema-version");
  if (jSchemaName == jj.end() || jSchemaVersion == jj.end())
  {
    vtkWarningMacro("No schema name and version provided. Skipping version checks.");
    skipVersionChecks = true;
  }
  if (!skipVersionChecks)
  {
    auto jFormatVersion = jj.find("format-version");
    if (jFormatVersion == jj.end() || jFormatVersion->get<std::uint32_t>() > 1)
    {
      vtkErrorMacro("File format version missing or newer than reader code.");
      return 0;
    }
    if (jSchemaName->get<std::string>() != "dg leaf")
    {
      vtkErrorMacro("Expecting a schema name of 'dg leaf'.");
      return 0;
    }
    if (jSchemaVersion->get<std::uint32_t>() > 1)
    {
      vtkErrorMacro("Cannot read a schema newer than v1.");
      return 0;
    }
    output->SetSchema(jSchemaName->get<std::string>(), jSchemaVersion->get<std::uint32_t>());
  }

  std::uint32_t contentVersion = 0;
  auto jContentVersion = jj.find("content-version");
  if (jContentVersion != jj.end())
  {
    contentVersion = jContentVersion->get<std::uint32_t>();
    output->SetContentVersion(contentVersion);
  }

  output->Initialize();

  for (const auto& jGroupEntry : jArrayGroup->items())
  {
    if (!jGroupEntry.value().is_array())
    {
      vtkWarningMacro("Skipping " << jGroupEntry.key());
      continue;
    }
    auto* arrayGroup = output->GetAttributes(vtkStringToken(jGroupEntry.key()).GetId());
    for (const auto& jArrayEntry : jGroupEntry.value().items())
    {
      auto arrayName = jArrayEntry.value()["name"].get<std::string>();
      auto arrayType = jArrayEntry.value()["type"].get<std::string>();
      auto arrayComps = jArrayEntry.value()["components"].get<int>();
      auto arrayTuples = jArrayEntry.value()["tuples"].get<vtkIdType>();
      auto* array = vtkDataArray::CreateDataArray(ArrayTypeToEnum(arrayType));
      array->SetNumberOfComponents(arrayComps);
      array->SetNumberOfTuples(arrayTuples);
      array->SetName(arrayName.c_str());
      switch (array->GetDataType())
      {
        vtkTemplateMacro(AppendArrayData(
          static_cast<VTK_TT*>(array->GetVoidPointer(0)), jArrayEntry.value()["data"]));
      }
      arrayGroup->AddArray(array);
      array->FastDelete();
      auto arrayIsScalars = jArrayEntry.value().find("default_scalars");
      if (arrayIsScalars != jArrayEntry.value().end() && arrayIsScalars->get<bool>())
      {
        arrayGroup->SetScalars(array);
      }
      auto arrayIsVec = jArrayEntry.value().find("default_vectors");
      if (arrayIsVec != jArrayEntry.value().end() && arrayIsVec->get<bool>())
      {
        arrayGroup->SetVectors(array);
      }
    }
  }

  for (const auto& jCellTypeEntry : *jCellTypes)
  {
    if (!jCellTypeEntry.is_object() || jCellTypeEntry.find("type") == jCellTypeEntry.end())
    {
      vtkWarningMacro("Skipping a malformed cell-type entry." << jCellTypeEntry.dump(2));
      continue;
    }
    auto cellType = vtkStringToken(jCellTypeEntry["type"].get<std::string>());
    auto cell = vtkCellMetadata::NewInstance(cellType, output);
    (void)cell;
  }

  for (const auto& jAttribute : *jAttributes)
  {
    if (!jAttribute.is_object() || jAttribute.find("name") == jAttribute.end() ||
      jAttribute.find("type") == jAttribute.end() || jAttribute.find("space") == jAttribute.end() ||
      jAttribute.find("components") == jAttribute.end() ||
      jAttribute.find("arrays") == jAttribute.end())
    {
      vtkWarningMacro("Skipping malformed cell-attribute entry. " << jAttribute.dump(2));
      continue;
    }
    auto attributeName = vtkStringToken(jAttribute["name"].get<std::string>());
    auto attributeType = vtkStringToken(jAttribute["type"].get<std::string>());
    auto attributeSpace = vtkStringToken(jAttribute["space"].get<std::string>());
    auto shapeIt = jAttribute.find("shape");
    bool attributeIsShape = !(shapeIt == jAttribute.end() || !shapeIt->get<bool>());
    auto attributeComps = jAttribute["components"].get<int>();
    vtkNew<vtkCellAttribute> attribute;
    attribute->Initialize(attributeName, attributeType, attributeSpace, attributeComps);
    for (const auto& arraySpecs : jAttribute["arrays"].items())
    {
      vtkStringToken cellTypeName(arraySpecs.key());
      vtkCellAttribute::ArraysForCellType arrays;
      for (const auto& arraySpec : arraySpecs.value().items())
      {
        vtkStringToken group(arraySpec.value()[0].get<std::string>());
        vtkStringToken arrayName(arraySpec.value()[1].get<std::string>());
        // std::cout << cellTypeName.Data() << " " << arraySpec.key() << " has " << group.Data() <<
        // ", " << arrayName.Data() << "\n";
        auto* arrayGroup = output->GetAttributes(group.GetId());
        if (arrayGroup)
        {
          auto* array = arrayGroup->GetArray(arrayName.Data().c_str());
          if (array)
          {
            arrays[arraySpec.key()] = array;
          }
        }
      }
      if (!arrays.empty())
      {
        attribute->SetArraysForCellType(cellTypeName, arrays);
      }
    }
    output->AddCellAttribute(attribute);
    if (attributeIsShape)
    {
      output->SetShapeAttribute(attribute);
    }
  }

  // Finally, although we have created vtkCellMetadata objects per the JSON,
  // we have not configured them. Now that the arrays and attributes are
  // present, use a query/responder to do so.
  vtkNew<vtkCellGridIOQuery> query;
  query->PrepareToDeserialize(*jCellTypes);
  if (!output->Query(query))
  {
    return 0;
  }

  return 1;
}
VTK_ABI_NAMESPACE_END
