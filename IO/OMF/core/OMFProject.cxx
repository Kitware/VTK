// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "OMFProject.h"
#include "OMFElement.h"
#include "OMFFile.h"
#include "OMFHelpers.h"

#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkInformation.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"

#include "vtk_jsoncpp.h"
#include "vtksys/SystemTools.hxx"

#include <memory>
#include <unordered_map>

namespace omf
{

namespace
{

enum class ElementTypes
{
  PointSetElement,
  LineSetElement,
  SurfaceElement,
  VolumeElement,
  InvalidElement
};

ElementTypes getElementTypeFromSubtype(const Json::Value& json)
{
  std::string subtype;
  helper::GetStringValue(json, subtype);
  static const std::unordered_map<std::string, ElementTypes> subtypeMap{
    { "point", ElementTypes::PointSetElement }, { "collar", ElementTypes::PointSetElement },
    { "blasthole", ElementTypes::PointSetElement }, { "line", ElementTypes::LineSetElement },
    { "borehole", ElementTypes::LineSetElement }, { "surface", ElementTypes::SurfaceElement },
    { "volume", ElementTypes::VolumeElement }
  };
  auto it = subtypeMap.find(subtype);
  return it == subtypeMap.end() ? ElementTypes::InvalidElement : it->second;
}

} // end anon namespace
VTK_ABI_NAMESPACE_BEGIN

struct OMFProject::ProjectImpl
{
  std::string UID;
  std::shared_ptr<OMFFile> ProjectFile;
  std::unordered_map<std::string, std::shared_ptr<ProjectElement>> Elements;
  using ElementMapType = std::unordered_map<std::string, std::shared_ptr<ProjectElement>>;

  void ProcessElement(const std::string& elementUID, vtkPartitionedDataSetCollection* output,
    vtkDataArraySelection* selection, bool writeOutTextures, bool columnMajorOrdering)
  {
    const auto& element = this->ProjectFile->JSONRoot()[elementUID];
    if (element.isNull() || !element.isObject())
    {
      vtkGenericWarningMacro(<< "element is null or not an object");
      return;
    }

    std::string name;
    helper::GetStringValue(element["name"], name);
    if (!selection->ArrayIsEnabled(name.c_str()))
    {
      return;
    }

    std::pair<ElementMapType::iterator, bool> elementResult;

    double globalOrigin[3];
    helper::GetPointFromJSON(this->ProjectFile->JSONRoot()[this->UID]["origin"], globalOrigin);

    if (element.isMember("subtype") && element["subtype"].isString())
    {
      auto subtype = getElementTypeFromSubtype(element["subtype"]);
      switch (subtype)
      {
        case ElementTypes::PointSetElement:
          elementResult = this->Elements.insert(
            std::make_pair(name, std::make_shared<PointSetElement>(elementUID, globalOrigin)));
          break;
        case ElementTypes::LineSetElement:
          elementResult = this->Elements.insert(
            std::make_pair(name, std::make_shared<LineSetElement>(elementUID, globalOrigin)));
          break;
        case ElementTypes::SurfaceElement:
          elementResult = this->Elements.insert(
            std::make_pair(name, std::make_shared<SurfaceElement>(elementUID, globalOrigin)));
          break;
        case ElementTypes::VolumeElement:
          elementResult = this->Elements.insert(
            std::make_pair(name, std::make_shared<VolumeElement>(elementUID, globalOrigin)));
          break;
        default:
          vtkGenericWarningMacro(<< "subtype " << element["subtype"].asString()
                                 << " is not a valid type");
          return;
      }

      vtkSmartPointer<vtkPartitionedDataSet> partitionedDS =
        vtkSmartPointer<vtkPartitionedDataSet>::New();
      elementResult.first->second->ProcessJSON(
        this->ProjectFile, element, partitionedDS, writeOutTextures, columnMajorOrdering);
      // names in vtkDataAssembly CANNOT contain spaces or parenthesis
      vtksys::SystemTools::ReplaceString(name, " ", "_");
      vtksys::SystemTools::ReplaceString(name, "(", "_");
      vtksys::SystemTools::ReplaceString(name, ")", "_");
      // also dashes are supposed to be fine to use in names, but there's a bug,
      // so replace dashes with underscores as well
      // see vtk issue #18128
      vtksys::SystemTools::ReplaceString(name, "-", "_");
      // names can only start with the following (see vtkDataAssembly::IsNodeNameValid)
      if ((name[0] < 'a' || name[0] > 'z') && (name[0] < 'A' || name[0] > 'Z') && name[0] != '_')
      {
        name = "_" + name;
      }
      auto assembly = output->GetDataAssembly();
      auto node = assembly->AddNode(name.c_str());
      vtkIdType pdsIdx = output->GetNumberOfPartitionedDataSets();
      assembly->AddDataSetIndex(node, pdsIdx);
      output->SetPartitionedDataSet(pdsIdx, partitionedDS);
      output->GetMetaData(pdsIdx)->Set(vtkCompositeDataSet::NAME(), name.c_str());
    }
  }

  bool GetDataElements(vtkDataArraySelection* selection)
  {
    const Json::Value& elements = this->ProjectFile->JSONRoot()[this->UID]["elements"];
    if (elements.isNull() || !elements.isArray())
    {
      vtkGenericWarningMacro(<< "Missing elements node under project node");
      return false;
    }

    for (Json::Value::ArrayIndex i = 0; i < elements.size(); ++i)
    {
      if (elements[i].isNull())
      {
        continue;
      }
      if (!elements[i].isString())
      {
        vtkGenericWarningMacro(<< "element " << i << " is not a string. It should be a UID.");
        continue;
      }
      std::string uid;
      helper::GetStringValue(elements[i], uid);
      const auto& element = this->ProjectFile->JSONRoot()[uid];
      std::string name;
      helper::GetStringValue(element["name"], name);
      selection->AddArray(name.c_str());
    }
    return true;
  }
};

//------------------------------------------------------------------------------
OMFProject::OMFProject()
  : Impl(new ProjectImpl())
{
}

//------------------------------------------------------------------------------
OMFProject::~OMFProject() = default;

//------------------------------------------------------------------------------
bool OMFProject::CanParseFile(const char* filename, vtkDataArraySelection* selection)
{
  if (!filename)
  {
    vtkGenericWarningMacro(<< "Input filename not specified");
    return false;
  }

  if (!this->Impl->ProjectFile)
  {
    this->Impl->ProjectFile = std::make_shared<OMFFile>();
  }

  if (!this->Impl->ProjectFile->OpenStream(filename))
  {
    vtkGenericWarningMacro(<< "Unable to open file " << filename);
    return false;
  }

  // read header to get location where JSON actually starts in file
  if (!this->Impl->ProjectFile->ReadHeader(this->Impl->UID))
  {
    return false;
  }

  if (!this->Impl->ProjectFile->ParseJSON())
  {
    return false;
  }

  if (this->Impl->ProjectFile->JSONRoot().empty())
  {
    vtkGenericWarningMacro(<< "root JSON object is empty");
    return false;
  }
  // all elements are stored in the json with a uid
  const Json::Value& projRoot = this->Impl->ProjectFile->JSONRoot()[this->Impl->UID];
  if (projRoot.isNull())
  {
    vtkGenericWarningMacro(<< "Missing project root node for UID " << this->Impl->UID);
    return false;
  }

  this->Impl->GetDataElements(selection);

  return true;
}

//------------------------------------------------------------------------------
bool OMFProject::ProcessJSON(vtkPartitionedDataSetCollection* output,
  vtkDataArraySelection* selection, bool writeOutTextures, bool columnMajorOrder)
{
  // loop thru elements and add processed elements to output

  // here we have the higher level elements of the data set
  // eg line set, point set, surfaces, etc
  // projRoot["elements"] only contains the uid of these elements
  const Json::Value& projRoot = this->Impl->ProjectFile->JSONRoot()[this->Impl->UID];
  if (projRoot.isNull())
  {
    vtkGenericWarningMacro(<< "Missing project root node for UID " << this->Impl->UID);
    return false;
  }

  std::string name;
  helper::GetStringValue(projRoot["name"], name);
  if (name.empty())
  {
    name = "OMF";
  }
  output->GetDataAssembly()->SetRootNodeName(name.c_str());

  const Json::Value& elements = projRoot["elements"];
  if (elements.isNull() || !elements.isArray())
  {
    vtkGenericWarningMacro(<< "ProcessJSON: Missing elements node under project node");
    return false;
  }

  for (Json::Value::ArrayIndex i = 0; i < elements.size(); ++i)
  {
    if (elements[i].isNull())
    {
      continue;
    }
    if (!elements[i].isString())
    {
      vtkGenericWarningMacro(<< "element " << i << " is not a string. It should be a UID.");
      continue;
    }
    std::string uid;
    helper::GetStringValue(elements[i], uid);
    this->Impl->ProcessElement(uid, output, selection, writeOutTextures, columnMajorOrder);
  }
  return true;
}

VTK_ABI_NAMESPACE_END
} // end namespace omf
