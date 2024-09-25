// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataAssemblyUtilities.h"

#include "vtkDataAssembly.h"
#include "vtkDataAssemblyVisitor.h"
#include "vtkDataObjectTypes.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSMPTools.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"

#include <cassert>
#include <functional>
#include <set>
#include <tuple>

VTK_ABI_NAMESPACE_BEGIN
namespace
{

std::pair<std::string, std::string> GetBlockNameAndLabel(
  vtkInformation* metadata, const std::string& defaultName)
{
  if (metadata && metadata->Has(vtkCompositeDataSet::NAME()) &&
    metadata->Get(vtkCompositeDataSet::NAME()))
  {
    std::string label = metadata->Get(vtkCompositeDataSet::NAME());
    if (!label.empty())
    {
      std::string name = vtkDataAssembly::MakeValidNodeName(label.c_str());
      return std::make_pair(name, label);
    }
  }
  return std::make_pair(defaultName, std::string());
}

// String used as the attribute name for data assembly nodes to identify
// data-assembly instances that represent a hierarchy.
constexpr const char* CATEGORY_ATTRIBUTE_NAME = "vtk_category";

// Value used for CATEGORY_ATTRIBUTE_NAME attribute on the generate data assembly
// when it represents the hierarchy for the input dataset.
constexpr const char* CATEGORY_HIERARCHY = "hierarchy";

// Value used for CATEGORY_ATTRIBUTE_NAME attribute on the generated data assembly
// when the data assembly representation a hierarchy for the input with dataset indices
// pointing to the transformed `vtkPartitionedDataSetCollection` rather than the input.
constexpr const char* CATEGORY_TRANSFORMED_HIERARCHY = "xformed_hierarchy";
}

vtkStandardNewMacro(vtkDataAssemblyUtilities);
//----------------------------------------------------------------------------
vtkDataAssemblyUtilities::vtkDataAssemblyUtilities() = default;

//----------------------------------------------------------------------------
vtkDataAssemblyUtilities::~vtkDataAssemblyUtilities() = default;

//----------------------------------------------------------------------------
void vtkDataAssemblyUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataAssembly> vtkDataAssemblyUtilities::GetDataAssembly(
  const char* name, vtkCompositeDataSet* cd)
{
  if (name == nullptr || cd == nullptr)
  {
    return nullptr;
  }
  else if (strcmp(name, vtkDataAssemblyUtilities::HierarchyName()) == 0)
  {
    vtkNew<vtkDataAssembly> assembly;
    if (vtkDataAssemblyUtilities::GenerateHierarchy(cd, assembly))
    {
      return assembly;
    }
  }
  else if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(cd))
  {
    // TODO: use name here; we don't support named assemblies yet on PDC.
    return pdc->GetDataAssembly();
  }

  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkDataAssemblyUtilities::GenerateHierarchy(vtkCompositeDataSet* input,
  vtkDataAssembly* hierarchy, vtkPartitionedDataSetCollection* output /*=nullptr*/)
{
  if (hierarchy == nullptr || input == nullptr)
  {
    return false;
  }

  hierarchy->Initialize();
  if (output)
  {
    output->Initialize();
  }
  if (auto amr = vtkUniformGridAMR::SafeDownCast(input))
  {
    return vtkDataAssemblyUtilities::GenerateHierarchyInternal(amr, hierarchy, output);
  }
  else if (auto mb = vtkMultiBlockDataSet::SafeDownCast(input))
  {
    return vtkDataAssemblyUtilities::GenerateHierarchyInternal(mb, hierarchy, output);
  }
  else if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(input))
  {
    return vtkDataAssemblyUtilities::GenerateHierarchyInternal(pdc, hierarchy, output);
  }

  return false;
}

//----------------------------------------------------------------------------
bool vtkDataAssemblyUtilities::GenerateHierarchyInternal(
  vtkUniformGridAMR* amr, vtkDataAssembly* hierarchy, vtkPartitionedDataSetCollection* output)
{
  assert(amr != nullptr && hierarchy != nullptr);
  hierarchy->SetRootNodeName("Root");
  hierarchy->SetAttribute(vtkDataAssembly::GetRootNode(), "vtk_type", amr->GetDataObjectType());
  hierarchy->SetAttribute(
    vtkDataAssembly::GetRootNode(), CATEGORY_ATTRIBUTE_NAME, CATEGORY_HIERARCHY);
  hierarchy->SetAttribute(vtkDataAssembly::GetRootNode(), "label", amr->GetClassName());

  if (output)
  {
    output->SetNumberOfPartitionedDataSets(amr->GetNumberOfLevels());
  }

  std::map<int, unsigned int> output_node2dataset_map;

  for (unsigned int level = 0, numLevels = amr->GetNumberOfLevels(); level < numLevels; ++level)
  {
    const auto label = "Level " + std::to_string(level);
    const auto name = vtkDataAssembly::MakeValidNodeName(label.c_str());
    auto node = hierarchy->AddNode(name.c_str());
    hierarchy->SetAttribute(node, "label", label.c_str());
    hierarchy->SetAttribute(node, "amr_level", level);

    const auto numDataSets = amr->GetNumberOfDataSets(level);
    // Add the composite index for each dataset in the AMR level.
    std::vector<unsigned int> datasetIndices(numDataSets);
    for (unsigned int cc = 0; cc < numDataSets; ++cc)
    {
      datasetIndices[cc] = amr->GetCompositeIndex(level, cc);
    }
    hierarchy->AddDataSetIndices(node, datasetIndices);
    hierarchy->SetAttribute(node, "number_of_datasets", numDataSets);
    if (output)
    {
      output_node2dataset_map[node] = level;
      output->SetNumberOfPartitions(level, numDataSets);
      for (unsigned int cc = 0; cc < numDataSets; ++cc)
      {
        output->SetPartition(level, cc, amr->GetDataSet(level, cc));
      }
      output->GetMetaData(level)->Set(
        vtkCompositeDataSet::NAME(), "Level " + std::to_string(level));
    }
  }

  if (output)
  {
    // if output is non-null, create a vtkDataAssembly that represents the
    // hierarchy for the input and update dataset indices in it to point to the
    // partitioned-dataset index in the output.
    vtkNew<vtkDataAssembly> clone;
    clone->DeepCopy(hierarchy);
    clone->SetAttribute(
      vtkDataAssembly::GetRootNode(), CATEGORY_ATTRIBUTE_NAME, CATEGORY_TRANSFORMED_HIERARCHY);
    clone->RemoveAllDataSetIndices(0, /*traverse_subtree=*/true);
    for (auto& pair : output_node2dataset_map)
    {
      clone->AddDataSetIndex(pair.first, pair.second);
    }
    output->SetDataAssembly(clone);
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkDataAssemblyUtilities::GenerateHierarchyInternal(
  vtkMultiBlockDataSet* input, vtkDataAssembly* hierarchy, vtkPartitionedDataSetCollection* output)
{
  assert(input != nullptr && hierarchy != nullptr);

  std::map<int, unsigned int> output_node2dataset_map;

  auto appendToOutput = [&](vtkDataObject* dobj, vtkInformation* metadata, int nodeid) {
    if (!output)
    {
      return;
    }

    const auto oid = output->GetNumberOfPartitionedDataSets();
    output->SetNumberOfPartitionedDataSets(oid + 1);
    output_node2dataset_map[nodeid] = oid;
    if (metadata)
    {
      output->GetMetaData(oid)->Copy(metadata);
    }
    if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(dobj))
    {
      output->GetPartitionedDataSet(oid)->CompositeShallowCopy(pdc);
    }
    else if (auto mp = vtkMultiPieceDataSet::SafeDownCast(dobj))
    {
      auto pd = output->GetPartitionedDataSet(oid);
      for (unsigned int cc = 0; cc < mp->GetNumberOfPieces(); ++cc)
      {
        pd->SetPartition(pd->GetNumberOfPartitions(), mp->GetPieceAsDataObject(cc));
      }
    }
    else if (dobj)
    {
      output->SetPartition(oid, 0, dobj);
    }
  };

  unsigned int cid = 0;
  std::function<void(vtkDataObject*, int, vtkInformation*)> f;
  f = [&](vtkDataObject* dobj, int nodeid, vtkInformation* dobjMetaData) {
    // in a hierarchy, the dataset-index corresponds to the composite index;
    // we add the "cid" attribute, however, to enable users to build selectors
    // using cid.
    hierarchy->SetAttribute(nodeid, "cid", cid);
    hierarchy->AddDataSetIndex(nodeid, cid);
    ++cid;

    if (auto mb = vtkMultiBlockDataSet::SafeDownCast(dobj))
    {
      hierarchy->SetAttribute(nodeid, "vtk_type", dobj->GetDataObjectType());
      for (unsigned int bidx = 0, numBlocks = mb->GetNumberOfBlocks(); bidx < numBlocks; ++bidx)
      {
        auto metadata = mb->HasMetaData(bidx) ? mb->GetMetaData(bidx) : nullptr;

        std::string label, name;
        std::tie(name, label) = ::GetBlockNameAndLabel(metadata, "Block" + std::to_string(bidx));

        auto child = hierarchy->AddNode(name.c_str(), nodeid);
        if (!label.empty())
        {
          hierarchy->SetAttribute(child, "label", label.c_str());
        }

        auto block = mb->GetBlock(bidx);
        f(block, child, metadata);
      }
    }
    else if (auto mp = vtkMultiPieceDataSet::SafeDownCast(dobj))
    {
      hierarchy->SetAttribute(nodeid, "vtk_type", dobj->GetDataObjectType());
      hierarchy->SetAttribute(nodeid, "vtk_num_pieces", mp->GetNumberOfPieces());
      appendToOutput(mp, dobjMetaData, nodeid);

      cid += mp->GetNumberOfPieces();
    }
    else
    {
      assert(vtkCompositeDataSet::SafeDownCast(dobj) == nullptr);
      // a leaf node.
      appendToOutput(dobj, dobjMetaData, nodeid);
    }
  };

  hierarchy->SetRootNodeName("Root");
  hierarchy->SetAttribute(
    vtkDataAssembly::GetRootNode(), CATEGORY_ATTRIBUTE_NAME, CATEGORY_HIERARCHY);
  hierarchy->SetAttribute(vtkDataAssembly::GetRootNode(), "label", input->GetClassName());
  f(input, vtkDataAssembly::GetRootNode(), nullptr);

  if (output)
  {
    // if output is non-null, create a vtkDataAssembly that represents the
    // hierarchy for the input and update dataset indices in it to point to the
    // partitioned-dataset index in the output.
    vtkNew<vtkDataAssembly> clone;
    clone->DeepCopy(hierarchy);
    clone->SetAttribute(
      vtkDataAssembly::GetRootNode(), CATEGORY_ATTRIBUTE_NAME, CATEGORY_TRANSFORMED_HIERARCHY);
    clone->RemoveAllDataSetIndices(0, /*traverse_subtree=*/true);
    for (auto& pair : output_node2dataset_map)
    {
      clone->AddDataSetIndex(pair.first, pair.second);
    }
    output->SetDataAssembly(clone);
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkDataAssemblyUtilities::GenerateHierarchyInternal(vtkPartitionedDataSetCollection* input,
  vtkDataAssembly* hierarchy, vtkPartitionedDataSetCollection* output)
{
  assert(input != nullptr && hierarchy != nullptr);
  if (output)
  {
    output->CompositeShallowCopy(input);
  }

  std::map<int, unsigned int> output_node2dataset_map;

  unsigned int cid = 0;
  hierarchy->SetRootNodeName("Root");
  hierarchy->SetAttribute(vtkDataAssembly::GetRootNode(), "vtk_type", input->GetDataObjectType());
  hierarchy->SetAttribute(
    vtkDataAssembly::GetRootNode(), CATEGORY_ATTRIBUTE_NAME, CATEGORY_HIERARCHY);
  hierarchy->SetAttribute(vtkDataAssembly::GetRootNode(), "label", input->GetClassName());
  hierarchy->AddDataSetIndex(vtkDataAssembly::GetRootNode(), cid++);

  for (unsigned int p = 0; p < input->GetNumberOfPartitionedDataSets(); ++p)
  {
    auto metadata = input->HasMetaData(p) ? input->GetMetaData(p) : nullptr;
    std::string name, label;
    std::tie(name, label) = ::GetBlockNameAndLabel(metadata, "Block" + std::to_string(p));

    auto node = hierarchy->AddNode(name.c_str());

    // dataset index in a hierarchy represents the composite index.
    hierarchy->AddDataSetIndex(node, cid++);

    if (!label.empty())
    {
      hierarchy->SetAttribute(node, "label", label.c_str());
    }

    hierarchy->SetAttribute(node, "number_of_partitions", input->GetNumberOfPartitions(p));

    if (output)
    {
      output_node2dataset_map[node] = p;
    }

    cid += input->GetNumberOfPartitions(p);
  }

  if (output)
  {
    // if output is non-null, create a vtkDataAssembly that represents the
    // hierarchy for the input and update dataset indices in it to point to the
    // partitioned-dataset index in the output.
    vtkNew<vtkDataAssembly> clone;
    clone->DeepCopy(hierarchy);
    clone->SetAttribute(
      vtkDataAssembly::GetRootNode(), CATEGORY_ATTRIBUTE_NAME, CATEGORY_TRANSFORMED_HIERARCHY);
    clone->RemoveAllDataSetIndices(0, /*traverse_subtree=*/true);
    for (auto& pair : output_node2dataset_map)
    {
      clone->AddDataSetIndex(pair.first, pair.second);
    }
    output->SetDataAssembly(clone);
  }

  return true;
}

namespace
{
class vtkVisitor : public vtkDataAssemblyVisitor
{
  vtkMultiBlockDataSet* Output;
  vtkPartitionedDataSetCollection* Input;
  std::vector<vtkMultiBlockDataSet*> Stack;

public:
  static vtkVisitor* New();
  vtkTypeMacro(vtkVisitor, vtkDataAssemblyVisitor);

  void SetOutput(vtkMultiBlockDataSet* mb) { this->Output = mb; }
  void SetInput(vtkPartitionedDataSetCollection* pdc) { this->Input = pdc; }

  void Visit(int nodeid) override
  {
    auto hierarchy = this->GetAssembly();
    const auto dataType = hierarchy->GetAttributeOrDefault(nodeid, "vtk_type", -1);
    if (nodeid == 0)
    {
      // sanity check.
      assert(dataType == this->Output->GetDataObjectType());
      this->Stack.push_back(this->Output);
      this->TraverseSubtree = true;
    }
    else if (dataType == VTK_MULTIBLOCK_DATA_SET)
    {
      assert(this->Stack.size() > 0);
      auto top = this->Stack.back();

      vtkNew<vtkMultiBlockDataSet> block;
      this->Stack.push_back(block);

      const unsigned int index = top->GetNumberOfBlocks();
      top->SetBlock(index, block);
      if (hierarchy->HasAttribute(nodeid, "label"))
      {
        top->GetMetaData(index)->Set(
          vtkCompositeDataSet::NAME(), hierarchy->GetAttributeOrDefault(nodeid, "label", ""));
      }
      this->TraverseSubtree = true;
    }
    else if (dataType == VTK_MULTIPIECE_DATA_SET)
    {
      assert(this->Stack.size() > 0);
      assert(hierarchy->GetNumberOfChildren(nodeid) == 0);
      auto top = this->Stack.back();

      vtkNew<vtkMultiPieceDataSet> mp;
      mp->SetNumberOfPieces(hierarchy->GetAttributeOrDefault(nodeid, "vtk_num_pieces", 0u));

      const unsigned int index = top->GetNumberOfBlocks();
      top->SetBlock(index, mp);
      if (hierarchy->HasAttribute(nodeid, "label"))
      {
        top->GetMetaData(index)->Set(
          vtkCompositeDataSet::NAME(), hierarchy->GetAttributeOrDefault(nodeid, "label", ""));
      }

      unsigned int next = 0;
      for (auto idx : this->GetCurrentDataSetIndices())
      {
        auto pd = this->Input->GetPartitionedDataSet(idx);
        for (unsigned int cc = 0; cc < pd->GetNumberOfPartitions(); ++cc)
        {
          mp->SetPartition(next++, pd->GetPartitionAsDataObject(cc));
        }
      }
      this->TraverseSubtree = false;
    }
    else
    {
      assert(this->Stack.size() > 0);
      assert(hierarchy->GetNumberOfChildren(nodeid) == 0);

      auto top = this->Stack.back();
      const unsigned int index = top->GetNumberOfBlocks();
      auto datasets = this->GetCurrentDataSetIndices();
      if (datasets.size() == 1)
      {
        const auto num_partitions = this->Input->GetNumberOfPartitions(datasets[0]);
        if (num_partitions == 1)
        {
          top->SetBlock(index, this->Input->GetPartitionAsDataObject(datasets[0], 0));
        }
        else if (num_partitions > 1)
        {
          // if more than 1 partition, we can't really put it in the output MB
          // without changing the structure of we only consider 1st one and warn
          // about it.
          vtkErrorMacro(
            "More than 1 partition encountered. Only 1st will be copied over (num_partitions="
            << num_partitions << ").");
          top->SetBlock(index, this->Input->GetPartitionAsDataObject(datasets[0], 0));
        }
        else
        {
          assert(num_partitions == 0);
          top->SetBlock(index, nullptr);
        }
      }
      else
      {
        assert(datasets.size() == 0);
        top->SetBlock(index, nullptr);
      }

      if (hierarchy->HasAttribute(nodeid, "label"))
      {
        top->GetMetaData(index)->Set(
          vtkCompositeDataSet::NAME(), hierarchy->GetAttributeOrDefault(nodeid, "label", ""));
      }
      this->TraverseSubtree = false;
    }
  }

  bool GetTraverseSubtree(int) override { return this->TraverseSubtree; }
  void BeginSubTree(int vtkNotUsed(nodeid)) override {}
  void EndSubTree(int vtkNotUsed(nodeid)) override
  {
    assert(this->Stack.size() > 0);
    this->Stack.pop_back();
  }

protected:
  vtkVisitor() = default;
  ~vtkVisitor() override = default;

private:
  vtkVisitor(const vtkVisitor&) = delete;
  void operator=(const vtkVisitor&) = delete;

  bool TraverseSubtree = false;
};
vtkStandardNewMacro(vtkVisitor);

}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkCompositeDataSet>
vtkDataAssemblyUtilities::GenerateCompositeDataSetFromHierarchy(
  vtkPartitionedDataSetCollection* input, vtkDataAssembly* hierarchy)
{
  if (!input || !hierarchy)
  {
    return nullptr;
  }

  const auto root = vtkDataAssembly::GetRootNode();

  if (strcmp(hierarchy->GetAttributeOrDefault(root, CATEGORY_ATTRIBUTE_NAME, ""),
        CATEGORY_TRANSFORMED_HIERARCHY) != 0)
  {
    vtkLogF(
      ERROR, "Input hierarchy not generated using `vtkDataAssemblyUtilities` is not supported!");
    return nullptr;
  }

  const auto dataType = hierarchy->GetAttributeOrDefault(root, "vtk_type", -1);
  if (vtkDataObjectTypes::TypeIdIsA(dataType, VTK_UNIFORM_GRID_AMR))
  {
    std::vector<int> blocks_per_level;
    for (const auto child : hierarchy->GetChildNodes(root, /*traverse_subtree=*/false))
    {
      auto level = hierarchy->GetAttributeOrDefault(child, "amr_level", 0u);
      const auto indices = hierarchy->GetDataSetIndices(child);
      assert(indices.size() == 0 || indices.size() == 1);

      const int count = indices.size() == 1 ? input->GetNumberOfPartitions(indices[0]) : 0;

      if (level >= static_cast<unsigned int>(blocks_per_level.size()))
      {
        blocks_per_level.resize(level + 1);
      }

      blocks_per_level[level] = count;
    }

    vtkSmartPointer<vtkUniformGridAMR> amr;
    amr.TakeReference(vtkUniformGridAMR::SafeDownCast(vtkDataObjectTypes::NewDataObject(dataType)));
    amr->Initialize(static_cast<int>(blocks_per_level.size()),
      !blocks_per_level.empty() ? blocks_per_level.data() : nullptr);
    for (const auto child : hierarchy->GetChildNodes(root, /*traverse_subtree=*/false))
    {
      auto level = hierarchy->GetAttributeOrDefault(child, "amr_level", 0u);
      const auto indices = hierarchy->GetDataSetIndices(child);
      assert(indices.size() == 0 || indices.size() == 1);
      if (indices.size() == 1)
      {
        for (unsigned int cc = 0, max = input->GetNumberOfPartitions(indices[0]); cc < max; ++cc)
        {
          amr->SetDataSet(
            level, cc, vtkUniformGrid::SafeDownCast(input->GetPartition(indices[0], cc)));
        }
      }
    }
    return amr;
  }

  else if (vtkDataObjectTypes::TypeIdIsA(dataType, VTK_MULTIBLOCK_DATA_SET))
  {
    vtkNew<vtkMultiBlockDataSet> mb;
    vtkNew<vtkVisitor> builder;
    builder->SetOutput(mb);
    builder->SetInput(input);
    hierarchy->Visit(builder, vtkDataAssembly::TraversalOrder::DepthFirst);
    return mb;
  }

  return nullptr;
}

//----------------------------------------------------------------------------
namespace
{
class vtkGenerateIdsVisitor : public vtkDataAssemblyVisitor
{
public:
  static vtkGenerateIdsVisitor* New();
  vtkTypeMacro(vtkGenerateIdsVisitor, vtkDataAssemblyVisitor);

  std::set<int> SelectedNodes;
  std::vector<unsigned int> CompositeIndices;

  void Visit(int nodeid) override
  {
    auto assembly = this->GetAssembly();
    assert(assembly->HasAttribute(nodeid, "cid"));
    const auto cid = assembly->GetAttributeOrDefault(nodeid, "cid", 0u);
    const auto type = assembly->GetAttributeOrDefault(nodeid, "vtk_type", 0);
    if (!this->EnabledStack.empty() ||
      this->SelectedNodes.find(nodeid) != this->SelectedNodes.end())
    {
      if (vtkDataObjectTypes::TypeIdIsA(type, VTK_COMPOSITE_DATA_SET))
      {
        if (vtkDataObjectTypes::TypeIdIsA(type, VTK_MULTIPIECE_DATA_SET))
        {
          // add indices for all parts.
          for (unsigned int cc = 0,
                            max = assembly->GetAttributeOrDefault(nodeid, "vtk_num_pieces", 0u);
               cc < max; ++cc)
          {
            this->CompositeIndices.push_back(cid + 1 + cc);
          }
        }
        else
        {
          // push to enable stack so that all our children are treated as
          // selected.
          this->EnabledStack.push_back(nodeid);
        }
      }
      else
      {
        this->CompositeIndices.push_back(cid);
      }
    }
  }

  void EndSubTree(int nodeid) override
  {
    if (!this->EnabledStack.empty() && this->EnabledStack.back() == nodeid)
    {
      this->EnabledStack.pop_back();
    }
  }

protected:
  vtkGenerateIdsVisitor() = default;
  ~vtkGenerateIdsVisitor() override = default;

private:
  vtkGenerateIdsVisitor(const vtkGenerateIdsVisitor&) = delete;
  void operator=(const vtkGenerateIdsVisitor&) = delete;

  std::vector<int> EnabledStack;
};
vtkStandardNewMacro(vtkGenerateIdsVisitor);

}

//----------------------------------------------------------------------------
std::vector<unsigned int> vtkDataAssemblyUtilities::GetSelectedCompositeIds(
  const std::vector<std::string>& selectors, vtkDataAssembly* hierarchyOrAssembly,
  vtkPartitionedDataSetCollection* data, bool leaf_nodes_only)
{
  if (hierarchyOrAssembly == nullptr || selectors.empty())
  {
    return {};
  }

  const auto root = vtkDataAssembly::GetRootNode();
  const bool isHierarchy =
    (strcmp(hierarchyOrAssembly->GetAttributeOrDefault(root, CATEGORY_ATTRIBUTE_NAME, ""),
       CATEGORY_HIERARCHY) == 0);
  if (!isHierarchy && data == nullptr)
  {
    vtkLogF(ERROR, "Missing required `data` argument.");
    return {};
  }

  if (isHierarchy && leaf_nodes_only)
  {
    const auto dataType = hierarchyOrAssembly->GetAttributeOrDefault(root, "vtk_type", -1);
    // for now we only support MBs. we could support AMR and PDC,
    // but I don't see the point in doing so right now.
    if (!vtkDataObjectTypes::TypeIdIsA(dataType, VTK_MULTIBLOCK_DATA_SET))
    {
      vtkLogF(ERROR, "Hierarchy does not represent a supported composite dataset type (%s)",
        vtkDataObjectTypes::GetClassNameFromTypeId(dataType));
      return {};
    }

    // the worst case: we need to traverse the hierarchy and determine composite
    // ids.
    const auto nodes = hierarchyOrAssembly->SelectNodes(selectors);
    vtkNew<vtkGenerateIdsVisitor> visitor;
    std::copy(nodes.begin(), nodes.end(),
      std::inserter(visitor->SelectedNodes, visitor->SelectedNodes.end()));
    hierarchyOrAssembly->Visit(visitor);
    return visitor->CompositeIndices;
  }

  // here, we only traverse the subtree if not a hierarchy. Otherwise, the
  // dataset indices are directly composite ids so we don't need to traverse
  // substree.
  auto dsIndices = hierarchyOrAssembly->GetDataSetIndices(
    hierarchyOrAssembly->SelectNodes(selectors), /*traverse_subtree=*/!isHierarchy);

  if (isHierarchy)
  {
    assert(leaf_nodes_only == false);

    // in this case, dsIndices directly correspond to the composite ids;
    // nothing more to do.
    return dsIndices;
  }
  else if (!isHierarchy && !leaf_nodes_only)
  {
    // convert partitioned dataset index to composite index.
    assert(data != nullptr);

    std::vector<unsigned int> cids(dsIndices.size());
    vtkSMPTools::Transform(dsIndices.begin(), dsIndices.end(), cids.begin(),
      [data](unsigned int partitionIdx) { return data->GetCompositeIndex(partitionIdx); });
    return cids;
  }
  else
  {
    assert(isHierarchy == false);
    assert(leaf_nodes_only == true);
    assert(data != nullptr);
    // convert partitioned dataset index to composite index for individual
    // partitions.
    std::vector<unsigned int> cids;
    for (const auto& partitionIdx : dsIndices)
    {
      for (unsigned int cc = 0; cc < data->GetNumberOfPartitions(partitionIdx); ++cc)
      {
        cids.push_back(data->GetCompositeIndex(partitionIdx, cc));
      }
    }
    return cids;
  }
}

//----------------------------------------------------------------------------
std::string vtkDataAssemblyUtilities::GetSelectorForCompositeId(
  unsigned int id, vtkDataAssembly* hierarchy)
{
  std::vector<unsigned int> ids;
  ids.push_back(id);

  auto selectors = vtkDataAssemblyUtilities::GetSelectorsForCompositeIds(ids, hierarchy);
  return selectors.empty() ? std::string() : selectors.front();
}

//----------------------------------------------------------------------------
namespace
{
//----------------------------------------------------------------------------
class vtkSelectorsForCompositeIdsVisitor : public vtkDataAssemblyVisitor
{
public:
  static vtkSelectorsForCompositeIdsVisitor* New();
  vtkTypeMacro(vtkSelectorsForCompositeIdsVisitor, vtkDataAssemblyVisitor);

  std::vector<std::string> Selectors;
  std::set<unsigned int> CompositeIds;

protected:
  vtkSelectorsForCompositeIdsVisitor() = default;
  ~vtkSelectorsForCompositeIdsVisitor() override = default;

  void Visit(int nodeid) override
  {
    const auto ids = this->GetCurrentDataSetIndices();
    if (ids.size() != 1)
    {
      // this happens for AMR if a level has no datasets.
      return;
    }

    const auto assembly = this->GetAssembly();
    const unsigned int cid = ids.front();
    unsigned int childCount = assembly->GetAttributeOrDefault(nodeid, "number_of_partitions", 0u);
    childCount = assembly->GetAttributeOrDefault(nodeid, "vtk_num_pieces", childCount);
    const auto cid_range = std::make_pair(cid, cid + 1 + childCount);
    for (auto id = cid_range.first; id < cid_range.second; ++id)
    {
      if (this->CompositeIds.find(id) != this->CompositeIds.end())
      {
        this->Selectors.push_back(assembly->GetNodePath(nodeid));
        break;
      }
    }
  }

private:
  vtkSelectorsForCompositeIdsVisitor(const vtkSelectorsForCompositeIdsVisitor&) = delete;
  void operator=(const vtkSelectorsForCompositeIdsVisitor&) = delete;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSelectorsForCompositeIdsVisitor);

//----------------------------------------------------------------------------
class vtkPartitionedDataSetIdsForCompositeIdsVisitor : public vtkDataAssemblyVisitor
{
public:
  static vtkPartitionedDataSetIdsForCompositeIdsVisitor* New();
  vtkTypeMacro(vtkPartitionedDataSetIdsForCompositeIdsVisitor, vtkDataAssemblyVisitor);

  std::vector<unsigned int> PartitionedDataSetIds;
  std::set<unsigned int> CompositeIds;

protected:
  vtkPartitionedDataSetIdsForCompositeIdsVisitor() = default;
  ~vtkPartitionedDataSetIdsForCompositeIdsVisitor() override = default;

  void Visit(int nodeid) override
  {
    const auto ids = this->GetCurrentDataSetIndices();
    if (ids.size() != 1)
    {
      // this happens for AMR if a level has no datasets.
      return;
    }

    const auto assembly = this->GetAssembly();
    const unsigned int partitionedDataSetId = assembly->GetAttributeOrDefault(nodeid, "id", 0u);
    const unsigned int cid = ids.front();
    unsigned int childCount = assembly->GetAttributeOrDefault(nodeid, "number_of_partitions", 0u);
    childCount = assembly->GetAttributeOrDefault(nodeid, "vtk_num_pieces", childCount);
    const auto cid_range = std::make_pair(cid, cid + 1 + childCount);
    for (auto id = cid_range.first; id < cid_range.second; ++id)
    {
      if (this->CompositeIds.find(id) != this->CompositeIds.end())
      {
        this->PartitionedDataSetIds.push_back(partitionedDataSetId - 1);
        break;
      }
    }
  }

private:
  vtkPartitionedDataSetIdsForCompositeIdsVisitor(
    const vtkPartitionedDataSetIdsForCompositeIdsVisitor&) = delete;
  void operator=(const vtkPartitionedDataSetIdsForCompositeIdsVisitor&) = delete;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPartitionedDataSetIdsForCompositeIdsVisitor);

//----------------------------------------------------------------------------
class vtkAssemblySelectorsForPartitionedDataSetIdsVisitor : public vtkDataAssemblyVisitor
{
public:
  static vtkAssemblySelectorsForPartitionedDataSetIdsVisitor* New();
  vtkTypeMacro(vtkAssemblySelectorsForPartitionedDataSetIdsVisitor, vtkDataAssemblyVisitor);

  std::set<unsigned int> PartitionedDataSetIds;
  std::vector<std::string> Selectors;

protected:
  vtkAssemblySelectorsForPartitionedDataSetIdsVisitor() = default;
  ~vtkAssemblySelectorsForPartitionedDataSetIdsVisitor() override = default;

  void Visit(int nodeid) override
  {
    const auto ids = this->GetCurrentDataSetIndices();
    for (auto id : ids)
    {
      if (this->PartitionedDataSetIds.find(id) != this->PartitionedDataSetIds.end())
      {
        this->Selectors.push_back(this->GetAssembly()->GetNodePath(nodeid));
        break;
      }
    }
  }

private:
  vtkAssemblySelectorsForPartitionedDataSetIdsVisitor(
    const vtkAssemblySelectorsForPartitionedDataSetIdsVisitor&) = delete;
  void operator=(const vtkAssemblySelectorsForPartitionedDataSetIdsVisitor&) = delete;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAssemblySelectorsForPartitionedDataSetIdsVisitor);

//----------------------------------------------------------------------------
class vtkSelectorsCompositeIdsForCompositeIdsVisitor : public vtkDataAssemblyVisitor
{
public:
  static vtkSelectorsCompositeIdsForCompositeIdsVisitor* New();
  vtkTypeMacro(vtkSelectorsCompositeIdsForCompositeIdsVisitor, vtkDataAssemblyVisitor);

  std::vector<unsigned int> SelectorsCompositeIds;
  std::set<unsigned int> CompositeIds;

protected:
  vtkSelectorsCompositeIdsForCompositeIdsVisitor() = default;
  ~vtkSelectorsCompositeIdsForCompositeIdsVisitor() override = default;

  void Visit(int nodeid) override
  {
    const auto ids = this->GetCurrentDataSetIndices();
    if (ids.size() != 1)
    {
      // this happens for AMR if a level has no datasets.
      return;
    }

    const auto assembly = this->GetAssembly();
    // get the composite id of this node.
    const unsigned int cid = ids.front();
    // get the number of partitions for this node.
    unsigned int childCount = assembly->GetAttributeOrDefault(nodeid, "number_of_partitions", 0u);
    // get the number of pieces for this node.
    childCount = assembly->GetAttributeOrDefault(nodeid, "vtk_num_pieces", childCount);
    // get the composite id range of the children of this node.
    const auto cid_range = std::make_pair(cid, cid + 1 + childCount);
    // iterate over the composite id range of the children of this node.
    for (auto id = cid_range.first; id < cid_range.second; ++id)
    {
      // if any of the children of this node is in the list of composite ids
      if (this->CompositeIds.find(id) != this->CompositeIds.end())
      {
        // add the composite id of this node to the list of selectors composite ids.
        this->SelectorsCompositeIds.push_back(cid);
        break;
      }
    }
  }

private:
  vtkSelectorsCompositeIdsForCompositeIdsVisitor(
    const vtkSelectorsCompositeIdsForCompositeIdsVisitor&) = delete;
  void operator=(const vtkSelectorsCompositeIdsForCompositeIdsVisitor&) = delete;
};

vtkStandardNewMacro(vtkSelectorsCompositeIdsForCompositeIdsVisitor);

} // namespace {}

//----------------------------------------------------------------------------
std::vector<std::string> vtkDataAssemblyUtilities::GetSelectorsForCompositeIds(
  const std::vector<unsigned int>& ids, vtkDataAssembly* hierarchy)
{
  const auto root = vtkDataAssembly::GetRootNode();
  if (strcmp(hierarchy->GetAttributeOrDefault(root, CATEGORY_ATTRIBUTE_NAME, ""),
        CATEGORY_HIERARCHY) != 0)
  {
    vtkLogF(ERROR,
      "GetSelectorForCompositeId is only supported on a data-assembly representation a hierarchy.");
    return {};
  }

  const auto dataType = hierarchy->GetAttributeOrDefault(root, "vtk_type", -1);
  if (vtkDataObjectTypes::TypeIdIsA(dataType, VTK_PARTITIONED_DATA_SET_COLLECTION) ||
    vtkDataObjectTypes::TypeIdIsA(dataType, VTK_MULTIBLOCK_DATA_SET))
  {
    vtkNew<vtkSelectorsForCompositeIdsVisitor> visitor;
    std::copy(
      ids.begin(), ids.end(), std::inserter(visitor->CompositeIds, visitor->CompositeIds.end()));
    hierarchy->Visit(visitor);
    return visitor->Selectors;
  }

  // in theory, this can work for AMR too, but I am leaving that until we have a
  // use-case.
  return {};
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkDataAssemblyUtilities::GetSelectorsForCompositeIds(
  const std::vector<unsigned int>& ids, vtkDataAssembly* hierarchy, vtkDataAssembly* assembly)
{
  const auto root = vtkDataAssembly::GetRootNode();
  if (strcmp(hierarchy->GetAttributeOrDefault(root, CATEGORY_ATTRIBUTE_NAME, ""),
        CATEGORY_HIERARCHY) != 0)
  {
    vtkLogF(ERROR, "hierarchy parameter should have attribute %s set to %s, but is '%s'",
      CATEGORY_ATTRIBUTE_NAME, CATEGORY_HIERARCHY,
      hierarchy->GetAttributeOrDefault(root, CATEGORY_ATTRIBUTE_NAME, ""));
    return {};
  }
  if (strcmp(assembly->GetAttributeOrDefault(root, CATEGORY_ATTRIBUTE_NAME, ""),
        CATEGORY_HIERARCHY) == 0)
  {
    vtkLogF(ERROR, "assembly parameter should have attribute %s not set to %s, but is '%s'",
      CATEGORY_ATTRIBUTE_NAME, CATEGORY_HIERARCHY,
      assembly->GetAttributeOrDefault(root, CATEGORY_ATTRIBUTE_NAME, ""));
    return {};
  }

  const auto dataType = hierarchy->GetAttributeOrDefault(root, "vtk_type", -1);
  if (vtkDataObjectTypes::TypeIdIsA(dataType, VTK_PARTITIONED_DATA_SET_COLLECTION))
  {
    vtkNew<vtkPartitionedDataSetIdsForCompositeIdsVisitor> visitor;
    std::copy(
      ids.begin(), ids.end(), std::inserter(visitor->CompositeIds, visitor->CompositeIds.end()));
    hierarchy->Visit(visitor);
    vtkNew<vtkAssemblySelectorsForPartitionedDataSetIdsVisitor> visitor2;
    std::copy(visitor->PartitionedDataSetIds.begin(), visitor->PartitionedDataSetIds.end(),
      std::inserter(visitor2->PartitionedDataSetIds, visitor2->PartitionedDataSetIds.end()));
    assembly->Visit(visitor2);
    return visitor2->Selectors;
  }
  return {};
}

//----------------------------------------------------------------------------
std::vector<unsigned int> vtkDataAssemblyUtilities::GetSelectorsCompositeIdsForCompositeIds(
  const std::vector<unsigned int>& ids, vtkDataAssembly* hierarchy)
{
  const auto root = vtkDataAssembly::GetRootNode();
  if (strcmp(hierarchy->GetAttributeOrDefault(root, CATEGORY_ATTRIBUTE_NAME, ""),
        CATEGORY_HIERARCHY) != 0)
  {
    vtkLogF(ERROR,
      "GetSelectorForCompositeId is only supported on a data-assembly representation a hierarchy.");
    return {};
  }

  const auto dataType = hierarchy->GetAttributeOrDefault(root, "vtk_type", -1);
  if (vtkDataObjectTypes::TypeIdIsA(dataType, VTK_PARTITIONED_DATA_SET_COLLECTION) ||
    vtkDataObjectTypes::TypeIdIsA(dataType, VTK_MULTIBLOCK_DATA_SET))
  {
    vtkNew<vtkSelectorsCompositeIdsForCompositeIdsVisitor> visitor;
    std::copy(
      ids.begin(), ids.end(), std::inserter(visitor->CompositeIds, visitor->CompositeIds.end()));
    hierarchy->Visit(visitor);
    return visitor->SelectorsCompositeIds;
  }

  // in theory, this can work for AMR too, but I am leaving that until we have a
  // use-case.
  return {};
}
VTK_ABI_NAMESPACE_END
