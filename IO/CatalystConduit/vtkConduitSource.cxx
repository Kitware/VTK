// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConduitSource.h"

#include "vtkConduitToDataObject.h"
#include "vtkConvertToMultiBlockDataSet.h"
#include "vtkDataAssembly.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>

#include <cassert>
#include <functional>
#include <map>

VTK_ABI_NAMESPACE_BEGIN

class vtkConduitSource::vtkInternals
{
public:
  conduit_cpp::Node Node;
  conduit_cpp::Node GlobalFieldsNode;
  conduit_cpp::Node AssemblyNode;
  bool GlobalFieldsNodeValid{ false };
  bool AssemblyNodeValid{ false };
};

vtkStandardNewMacro(vtkConduitSource);
//----------------------------------------------------------------------------
vtkConduitSource::vtkConduitSource()
  : Internals(new vtkConduitSource::vtkInternals())
  , UseAMRMeshProtocol(false)
  , UseMultiMeshProtocol(false)
  , OutputMultiBlock(false)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkConduitSource::~vtkConduitSource() = default;

//----------------------------------------------------------------------------
void vtkConduitSource::SetNode(const conduit_node* node)
{
  if (conduit_cpp::c_node(&this->Internals->Node) == node)
  {
    return;
  }
  this->Internals->Node = conduit_cpp::cpp_node(const_cast<conduit_node*>(node));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkConduitSource::SetGlobalFieldsNode(const conduit_node* node)
{
  if (this->Internals->GlobalFieldsNodeValid &&
    conduit_cpp::c_node(&this->Internals->GlobalFieldsNode) == node)
  {
    return;
  }

  if (node)
  {
    this->Internals->GlobalFieldsNode = conduit_cpp::cpp_node(const_cast<conduit_node*>(node));
  }
  this->Internals->GlobalFieldsNodeValid = (node != nullptr);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkConduitSource::SetAssemblyNode(const conduit_node* node)
{
  if (this->Internals->AssemblyNodeValid &&
    conduit_cpp::c_node(&this->Internals->AssemblyNode) == node)
  {
    return;
  }

  if (node)
  {
    this->Internals->AssemblyNode = conduit_cpp::cpp_node(const_cast<conduit_node*>(node));
  }
  this->Internals->AssemblyNodeValid = (node != nullptr);
  this->Modified();
}

//----------------------------------------------------------------------------
bool vtkConduitSource::GenerateAMR(vtkDataObject* output)
{
  vtkNew<vtkOverlappingAMR> amr_output;
  const auto& node = this->Internals->Node;

  if (!vtkConduitToDataObject::FillAMRMesh(amr_output, node))
  {
    vtkLogF(ERROR, "Failed reading AMR mesh '%s'", node.name().c_str());
    return false;
  }

  output->ShallowCopy(amr_output);
  return true;
}

//----------------------------------------------------------------------------
bool vtkConduitSource::GeneratePartitionedDataSet(vtkDataObject* output)
{
  vtkNew<vtkPartitionedDataSet> pd_output;
  if (!vtkConduitToDataObject::FillPartionedDataSet(pd_output, this->Internals->Node))
  {
    vtkLogF(ERROR, "Failed reading mesh from '%s'", this->Internals->Node.name().c_str());
    output->Initialize();
    return false;
  }

  output->ShallowCopy(pd_output);
  return true;
}

//----------------------------------------------------------------------------
bool vtkConduitSource::GeneratePartitionedDataSetCollection(vtkDataObject* output)
{
  vtkNew<vtkPartitionedDataSetCollection> pdc_output;
  const auto& pdc_node = this->Internals->Node;
  const auto count = pdc_node.number_of_children();
  pdc_output->SetNumberOfPartitionedDataSets(static_cast<unsigned int>(count));

  std::map<std::string, unsigned int> name_map;
  for (conduit_index_t cc = 0; cc < count; ++cc)
  {
    const auto& child = pdc_node.child(cc);
    auto pd = pdc_output->GetPartitionedDataSet(static_cast<unsigned int>(cc));
    assert(pd != nullptr);
    if (!vtkConduitToDataObject::FillPartionedDataSet(pd, child))
    {
      vtkLogF(ERROR, "Failed reading mesh '%s'", child.name().c_str());
      output->Initialize();
      return false;
    }

    // set the mesh name.
    pdc_output->GetMetaData(cc)->Set(vtkCompositeDataSet::NAME(), child.name().c_str());
    name_map[child.name()] = static_cast<unsigned int>(cc);

    // set field data.
    if (child.has_path("state/fields"))
    {
      vtkConduitToDataObject::AddFieldData(pd, child["state/fields"]);
    }
    // fields may be located in node at same level as state
    if (child.has_path("fields"))
    {
      vtkConduitToDataObject::AddFieldData(pd, child["fields"]);
    }
  }

  if (this->Internals->AssemblyNodeValid)
  {
    vtkNew<vtkDataAssembly> assembly;
    std::function<void(int, const conduit_cpp::Node&)> helper;
    helper = [&name_map, &assembly, &helper](int parent, const conduit_cpp::Node& node) {
      if (node.dtype().is_object())
      {
        for (conduit_index_t cc = 0; cc < node.number_of_children(); ++cc)
        {
          auto child = node.child(cc);
          auto nodeName = vtkDataAssembly::MakeValidNodeName(child.name().c_str());
          auto childId = assembly->AddNode(nodeName.c_str(), parent);
          assembly->SetAttribute(childId, "label", child.name().c_str());
          helper(childId, child);
        }
      }
      else if (node.dtype().is_list())
      {
        for (conduit_index_t cc = 0; cc < node.number_of_children(); ++cc)
        {
          auto child = node.child(cc);
          if (!child.dtype().is_string())
          {
            vtkLogF(ERROR, "list cannot have non-string items!");
            continue;
          }
          helper(parent, node.child(cc));
        }
      }
      else if (node.dtype().is_string())
      {
        auto value = node.as_string();
        auto iter = name_map.find(node.as_string());
        if (iter != name_map.end())
        {
          assembly->AddDataSetIndex(parent, iter->second);
        }
        else
        {
          vtkLogF(ERROR, "Assembly referring to unknown node '%s'. Skipping.", value.c_str());
        }
      }
    };
    // assembly->SetRootNodeName(....); What should this be?
    helper(assembly->GetRootNode(), this->Internals->AssemblyNode);
    pdc_output->SetDataAssembly(assembly);
  }

  output->ShallowCopy(pdc_output);

  return true;
}

//----------------------------------------------------------------------------
int vtkConduitSource::RequestDataObject(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  const int dataType = this->OutputMultiBlock ? VTK_MULTIBLOCK_DATA_SET
                                              : this->UseMultiMeshProtocol
      ? VTK_PARTITIONED_DATA_SET_COLLECTION
      : this->UseAMRMeshProtocol ? VTK_OVERLAPPING_AMR : VTK_PARTITIONED_DATA_SET;

  return this->SetOutputDataObject(dataType, outputVector->GetInformationObject(0), /*exact=*/true)
    ? 1
    : 0;
}

//----------------------------------------------------------------------------
int vtkConduitSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  auto& internals = (*this->Internals);
  vtkDataObject* real_output = vtkDataObject::GetData(outputVector, 0);

  bool dataGenerated = false;
  if (this->UseAMRMeshProtocol)
  {
    dataGenerated = this->GenerateAMR(real_output);
  }
  else if (this->UseMultiMeshProtocol)
  {
    dataGenerated = this->GeneratePartitionedDataSetCollection(real_output);
  }
  else
  {
    dataGenerated = this->GeneratePartitionedDataSet(real_output);
  }

  if (!dataGenerated)
  {
    return 0;
  }

  if (this->OutputMultiBlock)
  {
    vtkNew<vtkConvertToMultiBlockDataSet> converter;
    converter->SetInputData(real_output);
    converter->Update();
    real_output->ShallowCopy(converter->GetOutput());
  }

  if (internals.GlobalFieldsNodeValid)
  {
    vtkConduitToDataObject::AddFieldData(real_output, internals.GlobalFieldsNode);
  }

  if (internals.Node.has_path("state/fields"))
  {
    vtkConduitToDataObject::AddFieldData(real_output, internals.Node["state/fields"]);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkConduitSource::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
  {
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  if (!this->Internals->GlobalFieldsNodeValid)
  {
    return 1;
  }

  auto& node = this->Internals->GlobalFieldsNode;
  if (node.has_path("time"))
  {
    double time = node["time"].to_float64();
    double timesteps[2] = { time, time };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &time, 1);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timesteps, 2);
  }
  else
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkConduitSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
