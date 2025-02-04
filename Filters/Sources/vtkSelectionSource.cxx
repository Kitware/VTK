// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSelectionSource.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkUnsignedIntArray.h"

#include <algorithm>
#include <set>
#include <vector>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
struct vtkSelectionSource::NodeInformation
{
  using IDSetType = std::set<vtkIdType>;
  using IDsType = std::vector<IDSetType>;
  using StringSetType = std::set<std::string>;
  using StringIDsType = std::vector<StringSetType>;

  std::string Name;
  int ContentType;

  // data that defines the selection node
  std::string ArrayName;
  int ArrayComponent;
  IDsType IDs;
  StringIDsType StringIDs;
  std::vector<double> Thresholds;
  std::vector<double> Locations;
  double Frustum[32];
  IDSetType Blocks;
  StringSetType BlockSelectors;
  std::string QueryString;

  // Composite qualifiers
  int CompositeIndex;
  int HierarchicalLevel;
  int HierarchicalIndex;
  std::string AssemblyName;
  StringSetType Selectors;

  // Remaining qualifiers
  bool ContainingCells;
  bool Inverse;
  int NumberOfLayers;
  bool RemoveSeed;
  bool RemoveIntermediateLayers;

  NodeInformation()
    : ContentType(vtkSelectionNode::INDICES)
    , ArrayComponent(0)
    , CompositeIndex(-1)
    , HierarchicalLevel(-1)
    , HierarchicalIndex(-1)
    , ContainingCells(false)
    , Inverse(false)
    , NumberOfLayers(0)
    , RemoveSeed(false)
    , RemoveIntermediateLayers(false)
  {
    std::fill_n(this->Frustum, 32, 0);
  }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSelectionSource);

//------------------------------------------------------------------------------
vtkSelectionSource::vtkSelectionSource()
  : FieldTypeOption(FieldTypeOptions::FIELD_TYPE)
  , FieldType(vtkSelectionNode::CELL)
  , ElementType(vtkDataObject::CELL)
  , ProcessID(-1)
{
  this->NodesInfo.push_back(std::make_shared<NodeInformation>());
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkSelectionSource::~vtkSelectionSource()
{
  this->NodesInfo.clear();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetNumberOfNodes(unsigned int numberOfNodes)
{
  if (numberOfNodes == this->NodesInfo.size())
  {
    return;
  }

  this->NodesInfo.resize(numberOfNodes);
  for (auto& nodeInfo : this->NodesInfo)
  {
    if (!nodeInfo)
    {
      nodeInfo = std::make_shared<NodeInformation>();
    }
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::RemoveNode(unsigned int idx)
{
  if (idx >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << idx);
    return;
  }
  this->NodesInfo.erase(this->NodesInfo.begin() + idx);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::RemoveNode(const char* name)
{
  for (unsigned int i = 0; i < this->GetNumberOfNodes(); ++i)
  {
    if (this->NodesInfo[i]->Name == name)
    {
      this->RemoveNode(i--);
      return;
    }
  }
}

//------------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllNodes()
{
  this->NodesInfo.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetNodeName(unsigned int nodeId, const char* name)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  const std::string safeNodeName = name ? name : "";
  if (safeNodeName.empty())
  {
    return;
  }
  if (std::find_if(this->NodesInfo.begin(), this->NodesInfo.end(),
        [&safeNodeName](const std::shared_ptr<NodeInformation>& nodeInfo)
        { return nodeInfo->Name == safeNodeName; }) != this->NodesInfo.end())
  {
    vtkErrorMacro("Node name already exists: " << safeNodeName);
    return;
  }
  if (this->NodesInfo[nodeId]->Name != safeNodeName)
  {
    this->NodesInfo[nodeId]->Name = safeNodeName;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
const char* vtkSelectionSource::GetNodeName(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return nullptr;
  }
  return this->NodesInfo[nodeId]->Name.c_str();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllIDs(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  this->NodesInfo[nodeId]->IDs.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllStringIDs(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  this->NodesInfo[nodeId]->StringIDs.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllLocations(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  this->NodesInfo[nodeId]->Locations.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllThresholds(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  this->NodesInfo[nodeId]->Thresholds.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::AddID(unsigned int nodeId, vtkIdType proc, vtkIdType id)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  // proc == -1 means all processes. All others are stored at index proc+1
  proc++;

  if (proc >= (vtkIdType)this->NodesInfo[nodeId]->IDs.size())
  {
    this->NodesInfo[nodeId]->IDs.resize(proc + 1);
  }
  this->NodesInfo[nodeId]->IDs[proc].insert(id);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::AddStringID(unsigned int nodeId, vtkIdType proc, const char* id)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  const std::string safeId = id ? id : "";
  if (safeId.empty())
  {
    return;
  }
  // proc == -1 means all processes. All others are stored at index proc+1
  proc++;

  if (proc >= (vtkIdType)this->NodesInfo[nodeId]->StringIDs.size())
  {
    this->NodesInfo[nodeId]->StringIDs.resize(proc + 1);
  }
  this->NodesInfo[nodeId]->StringIDs[proc].insert(safeId);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::AddLocation(unsigned int nodeId, double x, double y, double z)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  this->NodesInfo[nodeId]->Locations.push_back(x);
  this->NodesInfo[nodeId]->Locations.push_back(y);
  this->NodesInfo[nodeId]->Locations.push_back(z);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::AddThreshold(unsigned int nodeId, double min, double max)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  this->NodesInfo[nodeId]->Thresholds.push_back(min);
  this->NodesInfo[nodeId]->Thresholds.push_back(max);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetFrustum(unsigned int nodeId, double* vertices)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  for (int cc = 0; cc < 32; cc++)
  {
    if (vertices[cc] != this->NodesInfo[nodeId]->Frustum[cc])
    {
      memcpy(this->NodesInfo[nodeId]->Frustum, vertices, 32 * sizeof(double));
      this->Modified();
      break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkSelectionSource::AddBlock(unsigned int nodeId, vtkIdType block)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  this->NodesInfo[nodeId]->Blocks.insert(block);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllBlocks(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  this->NodesInfo[nodeId]->Blocks.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::AddBlockSelector(unsigned int nodeId, const char* selector)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  const std::string safeBlockSelectorName = selector ? selector : "";
  if (safeBlockSelectorName.empty())
  {
    return;
  }
  this->NodesInfo[nodeId]->BlockSelectors.insert(safeBlockSelectorName);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllBlockSelectors(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  if (!this->NodesInfo[nodeId]->BlockSelectors.empty())
  {
    this->NodesInfo[nodeId]->BlockSelectors.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetContentType(unsigned int nodeId, int contentType)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  contentType =
    std::min<int>(std::max<int>(contentType, vtkSelectionNode::SelectionContent::GLOBALIDS),
      vtkSelectionNode::SelectionContent::USER);
  if (this->NodesInfo[nodeId]->ContentType != contentType)
  {
    this->NodesInfo[nodeId]->ContentType = contentType;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkSelectionSource::GetContentType(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return 0;
  }
  return this->NodesInfo[nodeId]->ContentType;
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetContainingCells(unsigned int nodeId, vtkTypeBool containingCells)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  if (this->NodesInfo[nodeId]->ContainingCells != static_cast<bool>(containingCells))
  {
    this->NodesInfo[nodeId]->ContainingCells = static_cast<bool>(containingCells);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkTypeBool vtkSelectionSource::GetContainingCells(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return false;
  }
  return static_cast<vtkTypeBool>(this->NodesInfo[nodeId]->ContainingCells);
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetNumberOfLayers(unsigned int nodeId, int numberOfLayers)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  numberOfLayers = std::min(std::max(numberOfLayers, 0), VTK_INT_MAX);
  if (this->NodesInfo[nodeId]->NumberOfLayers != numberOfLayers)
  {
    this->NodesInfo[nodeId]->NumberOfLayers = numberOfLayers;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkSelectionSource::GetNumberOfLayers(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return 0;
  }
  return this->NodesInfo[nodeId]->NumberOfLayers;
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetRemoveSeed(unsigned int nodeId, bool removeSeed)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  if (this->NodesInfo[nodeId]->RemoveSeed != removeSeed)
  {
    this->NodesInfo[nodeId]->RemoveSeed = removeSeed;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
bool vtkSelectionSource::GetRemoveSeed(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return false;
  }
  return this->NodesInfo[nodeId]->RemoveSeed;
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetRemoveIntermediateLayers(
  unsigned int nodeId, bool removeIntermediateLayers)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  if (this->NodesInfo[nodeId]->RemoveIntermediateLayers != removeIntermediateLayers)
  {
    this->NodesInfo[nodeId]->RemoveIntermediateLayers = removeIntermediateLayers;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
bool vtkSelectionSource::GetRemoveIntermediateLayers(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return false;
  }
  return this->NodesInfo[nodeId]->RemoveIntermediateLayers;
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetInverse(unsigned int nodeId, vtkTypeBool inverse)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  if (this->NodesInfo[nodeId]->Inverse != static_cast<bool>(inverse))
  {
    this->NodesInfo[nodeId]->Inverse = static_cast<bool>(inverse);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkTypeBool vtkSelectionSource::GetInverse(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return false;
  }
  return static_cast<vtkTypeBool>(this->NodesInfo[nodeId]->Inverse);
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetArrayName(unsigned int nodeId, const char* arrayName)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  const std::string safeArrayName = arrayName ? arrayName : "";
  if (safeArrayName.empty())
  {
    return;
  }
  if (this->NodesInfo[nodeId]->ArrayName != safeArrayName)
  {
    this->NodesInfo[nodeId]->ArrayName = safeArrayName;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
const char* vtkSelectionSource::GetArrayName(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return nullptr;
  }
  return this->NodesInfo[nodeId]->ArrayName.c_str();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetArrayComponent(unsigned int nodeId, int arrayComponent)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  if (this->NodesInfo[nodeId]->ArrayComponent != arrayComponent)
  {
    this->NodesInfo[nodeId]->ArrayComponent = arrayComponent;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkSelectionSource::GetArrayComponent(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return 0;
  }
  return this->NodesInfo[nodeId]->ArrayComponent;
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetCompositeIndex(unsigned int nodeId, int index)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  if (this->NodesInfo[nodeId]->CompositeIndex != index)
  {
    this->NodesInfo[nodeId]->CompositeIndex = index;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkSelectionSource::GetCompositeIndex(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return 0;
  }
  return this->NodesInfo[nodeId]->CompositeIndex;
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetHierarchicalLevel(unsigned int nodeId, int level)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  if (this->NodesInfo[nodeId]->HierarchicalLevel != level)
  {
    this->NodesInfo[nodeId]->HierarchicalLevel = level;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkSelectionSource::GetHierarchicalLevel(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return 0;
  }
  return this->NodesInfo[nodeId]->HierarchicalLevel;
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetHierarchicalIndex(unsigned int nodeId, int index)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  if (this->NodesInfo[nodeId]->HierarchicalIndex != index)
  {
    this->NodesInfo[nodeId]->HierarchicalIndex = index;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkSelectionSource::GetHierarchicalIndex(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return 0;
  }
  return this->NodesInfo[nodeId]->HierarchicalIndex;
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetAssemblyName(unsigned int nodeId, const char* name)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  const std::string safeAssemblyName = name ? name : "";
  if (safeAssemblyName.empty())
  {
    return;
  }
  if (this->NodesInfo[nodeId]->AssemblyName != safeAssemblyName)
  {
    this->NodesInfo[nodeId]->AssemblyName = safeAssemblyName;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
const char* vtkSelectionSource::GetAssemblyName(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return nullptr;
  }
  return this->NodesInfo[nodeId]->AssemblyName.c_str();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::AddSelector(unsigned int nodeId, const char* selector)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  const std::string safeSelectorName = selector ? selector : "";
  if (safeSelectorName.empty())
  {
    return;
  }
  this->NodesInfo[nodeId]->Selectors.insert(safeSelectorName);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllSelectors(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  if (!this->NodesInfo[nodeId]->Selectors.empty())
  {
    this->NodesInfo[nodeId]->Selectors.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkSelectionSource::SetQueryString(unsigned int nodeId, const char* queryString)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return;
  }
  const std::string safeQueryString = queryString ? queryString : "";
  if (safeQueryString.empty())
  {
    return;
  }
  if (this->NodesInfo[nodeId]->QueryString != safeQueryString)
  {
    this->NodesInfo[nodeId]->QueryString = safeQueryString;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
const char* vtkSelectionSource::GetQueryString(unsigned int nodeId)
{
  if (nodeId >= this->NodesInfo.size())
  {
    vtkErrorMacro("Invalid node id: " << nodeId);
    return nullptr;
  }
  return this->NodesInfo[nodeId]->QueryString.c_str();
}

//------------------------------------------------------------------------------
void vtkSelectionSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldTypeOption: "
     << (this->FieldTypeOption == FieldTypeOptions::FIELD_TYPE ? "FieldType" : "ElementType")
     << endl;
  if (this->FieldTypeOption == FieldTypeOptions::FIELD_TYPE)
  {
    os << indent << "FieldType: " << vtkSelectionNode::GetFieldTypeAsString(this->FieldType)
       << endl;
  }
  else
  {
    os << indent << "ElementType: "
       << vtkSelectionNode::GetFieldTypeAsString(
            vtkSelectionNode::ConvertAttributeTypeToSelectionField(this->ElementType))
       << endl;
  }
  os << indent << "ProcessID: " << this->ProcessID << endl;
  for (const auto& nodeInfo : this->NodesInfo)
  {
    os << indent
       << "ContentType: " << vtkSelectionNode::GetContentTypeAsString(nodeInfo->ContentType)
       << endl;
    os << indent << "ContainingCells: " << (nodeInfo->ContainingCells ? "Yes" : "No") << endl;
    os << indent << "Inverse: " << (nodeInfo->Inverse ? "Yes" : "No") << endl;
    os << indent << "ArrayName: " << nodeInfo->ArrayName << endl;
    os << indent << "ArrayComponent: " << nodeInfo->ArrayComponent << endl;
    os << indent << "CompositeIndex: " << nodeInfo->CompositeIndex << endl;
    os << indent << "HierarchicalLevel: " << nodeInfo->HierarchicalLevel << endl;
    os << indent << "HierarchicalIndex: " << nodeInfo->HierarchicalIndex << endl;
    os << indent << "QueryString: " << nodeInfo->QueryString << endl;
    os << indent << "NumberOfLayers: " << nodeInfo->NumberOfLayers << endl;
    os << indent << "AssemblyName: " << nodeInfo->AssemblyName << endl;
  }
}

//------------------------------------------------------------------------------
int vtkSelectionSource::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//------------------------------------------------------------------------------
int vtkSelectionSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkSelection* outputSel = vtkSelection::GetData(outputVector);
  if (!this->Expression.empty())
  {
    outputSel->SetExpression(this->Expression);
  }
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece = outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER())
    ? outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER())
    : 0;

  int fieldType = this->FieldTypeOption == FieldTypeOptions::FIELD_TYPE
    ? this->FieldType
    : vtkSelectionNode::ConvertAttributeTypeToSelectionField(this->ElementType);

  for (unsigned int nodeId = 0; nodeId < this->GetNumberOfNodes(); ++nodeId)
  {
    const auto& nodeInfo = this->NodesInfo[nodeId];
    vtkNew<vtkSelectionNode> node;
    if (!nodeInfo->Name.empty())
    {
      outputSel->SetNode(nodeInfo->Name, node);
    }
    else
    {
      outputSel->AddNode(node);
    }
    vtkInformation* oProperties = node->GetProperties();

    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
      if (this->ProcessID >= 0)
      {
        oProperties->Set(vtkSelectionNode::PROCESS_ID(), this->ProcessID);
      }
    }
    if (nodeInfo->CompositeIndex >= 0)
    {
      oProperties->Set(vtkSelectionNode::COMPOSITE_INDEX(), nodeInfo->CompositeIndex);
    }

    if (nodeInfo->HierarchicalLevel >= 0 && nodeInfo->HierarchicalIndex >= 0)
    {
      oProperties->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(), nodeInfo->HierarchicalLevel);
      oProperties->Set(vtkSelectionNode::HIERARCHICAL_INDEX(), nodeInfo->HierarchicalIndex);
    }

    if (!nodeInfo->AssemblyName.empty() && !nodeInfo->Selectors.empty())
    {
      oProperties->Set(vtkSelectionNode::ASSEMBLY_NAME(), nodeInfo->AssemblyName.c_str());
      for (auto& selector : nodeInfo->Selectors)
      {
        oProperties->Append(vtkSelectionNode::SELECTORS(), selector.c_str());
      }
    }

    switch (nodeInfo->ContentType)
    {
      case vtkSelectionNode::GLOBALIDS:
      case vtkSelectionNode::PEDIGREEIDS:
      case vtkSelectionNode::INDICES:
      case vtkSelectionNode::VALUES:
      {
        // First look for string ids.
        if (!nodeInfo->StringIDs.empty())
        {
          oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), nodeInfo->ContentType);
          oProperties->Set(vtkSelectionNode::FIELD_TYPE(), fieldType);

          vtkNew<vtkStringArray> selectionList;
          node->SetSelectionList(selectionList);

          // Number of selected items common to all pieces
          vtkIdType numCommonElems = 0;
          if (!nodeInfo->StringIDs.empty())
          {
            numCommonElems = static_cast<vtkIdType>(nodeInfo->StringIDs[0].size());
          }
          if (piece + 1 >= (int)nodeInfo->StringIDs.size() && numCommonElems == 0)
          {
            vtkDebugMacro("No selection for piece: " << piece);
          }
          else
          {
            // idx == 0 is the list for all pieces
            // idx == piece+1 is the list for the current piece
            size_t pids[2];
            pids[0] = 0;
            pids[1] = static_cast<size_t>(piece + 1);
            for (int i = 0; i < 2; i++)
            {
              size_t idx = pids[i];
              if (idx >= nodeInfo->StringIDs.size())
              {
                continue;
              }

              auto& selSet = nodeInfo->StringIDs[idx];

              if (!selSet.empty())
              {
                // Create the selection list
                selectionList->SetNumberOfTuples(static_cast<vtkIdType>(selSet.size()));
                // iterate over ids and insert to the selection list
                vtkIdType idx2 = 0;
                for (const auto& id : selSet)
                {
                  selectionList->SetValue(idx2++, id);
                }
              }
            }
          }
        }
        // If no string ids, use integer ids.
        else
        {
          oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), nodeInfo->ContentType);
          oProperties->Set(vtkSelectionNode::FIELD_TYPE(), fieldType);

          vtkNew<vtkIdTypeArray> selectionList;
          node->SetSelectionList(selectionList);

          // Number of selected items common to all pieces
          vtkIdType numCommonElems = 0;
          if (!nodeInfo->IDs.empty())
          {
            numCommonElems = static_cast<vtkIdType>(nodeInfo->IDs[0].size());
          }
          if (piece + 1 >= (int)nodeInfo->IDs.size() && numCommonElems == 0)
          {
            vtkDebugMacro("No selection for piece: " << piece);
          }
          else
          {
            // idx == 0 is the list for all pieces
            // idx == piece+1 is the list for the current piece
            size_t pids[2] = { static_cast<size_t>(0), static_cast<size_t>(piece + 1) };
            for (int i = 0; i < 2; i++)
            {
              size_t idx = pids[i];
              if (idx >= nodeInfo->IDs.size())
              {
                continue;
              }

              auto& selSet = nodeInfo->IDs[idx];

              if (!selSet.empty())
              {
                // Create the selection list
                selectionList->SetNumberOfTuples(static_cast<vtkIdType>(selSet.size()));
                // iterate over ids and insert to the selection list
                vtkIdType idx2 = 0;
                for (const auto& id : selSet)
                {
                  selectionList->SetValue(idx2++, id);
                }
              }
            }
          }
        }
        break;
      }
      case vtkSelectionNode::LOCATIONS:
      {
        oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), nodeInfo->ContentType);
        oProperties->Set(vtkSelectionNode::FIELD_TYPE(), fieldType);
        // Create the selection list
        vtkNew<vtkDoubleArray> selectionList;
        selectionList->SetNumberOfComponents(3);
        selectionList->SetNumberOfValues(static_cast<vtkIdType>(nodeInfo->Locations.size()));

        vtkIdType cc = 0;
        for (const auto& locationCoordinate : nodeInfo->Locations)
        {
          selectionList->SetValue(cc++, locationCoordinate);
        }
        node->SetSelectionList(selectionList);
        break;
      }
      case vtkSelectionNode::THRESHOLDS:
      {
        oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), nodeInfo->ContentType);
        oProperties->Set(vtkSelectionNode::FIELD_TYPE(), fieldType);
        oProperties->Set(vtkSelectionNode::COMPONENT_NUMBER(), nodeInfo->ArrayComponent);
        // Create the selection list
        vtkNew<vtkDoubleArray> selectionList;
        selectionList->SetNumberOfComponents(2);
        selectionList->SetNumberOfValues(static_cast<vtkIdType>(nodeInfo->Thresholds.size()));

        auto iter = nodeInfo->Thresholds.begin();
        for (vtkIdType cc = 0; iter != nodeInfo->Thresholds.end(); ++iter, ++cc)
        {
          selectionList->SetTypedComponent(cc, 0, *iter);
          ++iter;
          selectionList->SetTypedComponent(cc, 1, *iter);
        }
        node->SetSelectionList(selectionList);
        break;
      }
      case vtkSelectionNode::FRUSTUM:
      {
        oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), nodeInfo->ContentType);
        oProperties->Set(vtkSelectionNode::FIELD_TYPE(), fieldType);
        // Create the selection list
        vtkNew<vtkDoubleArray> selectionList;
        selectionList->SetNumberOfComponents(4);
        selectionList->SetNumberOfTuples(8);
        for (vtkIdType cc = 0; cc < 32; cc++)
        {
          selectionList->SetValue(cc, nodeInfo->Frustum[cc]);
        }
        node->SetSelectionList(selectionList);
        break;
      }
      case vtkSelectionNode::BLOCKS:
      {
        oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), nodeInfo->ContentType);
        oProperties->Set(vtkSelectionNode::FIELD_TYPE(), fieldType);
        vtkNew<vtkUnsignedIntArray> selectionList;
        selectionList->SetNumberOfComponents(1);
        selectionList->SetNumberOfTuples(static_cast<vtkIdType>(nodeInfo->Blocks.size()));
        vtkIdType cc = 0;
        for (const auto& block : nodeInfo->Blocks)
        {
          selectionList->SetValue(cc++, block);
        }
        node->SetSelectionList(selectionList);
        break;
      }
      case vtkSelectionNode::BLOCK_SELECTORS:
      {
        oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), nodeInfo->ContentType);
        oProperties->Set(vtkSelectionNode::FIELD_TYPE(), fieldType);
        vtkNew<vtkStringArray> selectionList;
        selectionList->SetNumberOfTuples(static_cast<vtkIdType>(nodeInfo->BlockSelectors.size()));
        vtkIdType cc = 0;
        for (const auto& selector : nodeInfo->BlockSelectors)
        {
          selectionList->SetValue(cc++, selector);
        }
        node->SetSelectionList(selectionList);
        break;
      }
      case vtkSelectionNode::QUERY:
      {
        oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), nodeInfo->ContentType);
        oProperties->Set(vtkSelectionNode::FIELD_TYPE(), fieldType);
        node->SetQueryString(nodeInfo->QueryString.c_str());
        break;
      }
      case vtkSelectionNode::USER:
      {
        vtkErrorMacro("User-supplied, application-specific selections are not supported.");
        return 0;
      }
      default:
      {
        vtkErrorMacro("Unsupported content type: " << nodeInfo->ContentType);
        return 0;
      }
    }

    oProperties->Set(vtkSelectionNode::CONTAINING_CELLS(), nodeInfo->ContainingCells);

    oProperties->Set(vtkSelectionNode::INVERSE(), nodeInfo->Inverse);

    if (node->GetSelectionList() && !nodeInfo->ArrayName.empty())
    {
      node->GetSelectionList()->SetName(nodeInfo->ArrayName.c_str());
    }
    oProperties->Set(vtkSelectionNode::CONNECTED_LAYERS(), nodeInfo->NumberOfLayers);
    oProperties->Set(
      vtkSelectionNode::CONNECTED_LAYERS_REMOVE_SEED(), nodeInfo->RemoveSeed ? 1 : 0);
    oProperties->Set(vtkSelectionNode::CONNECTED_LAYERS_REMOVE_INTERMEDIATE_LAYERS(),
      nodeInfo->RemoveIntermediateLayers ? 1 : 0);
  }
  return 1;
}
VTK_ABI_NAMESPACE_END
