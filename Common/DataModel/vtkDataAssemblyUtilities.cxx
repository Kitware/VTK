/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataAssemblyUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"

#include <cassert>
#include <functional>
#include <iterator>
#include <set>

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
  hierarchy->SetAttribute(vtkDataAssembly::GetRootNode(), "vtk_category", "hierarchy");

  if (output)
  {
    output->SetNumberOfPartitionedDataSets(amr->GetNumberOfLevels());
  }

  for (unsigned int level = 0, numLevels = amr->GetNumberOfLevels(); level < numLevels; ++level)
  {
    auto node = hierarchy->AddNode(("Block" + std::to_string(level)).c_str());
    hierarchy->SetAttribute(node, "label", ("Level " + std::to_string(level)).c_str());
    hierarchy->SetAttribute(node, "amr_level", level);

    const auto numDataSets = amr->GetNumberOfDataSets(level);
    hierarchy->SetAttribute(node, "number_of_datasets", numDataSets);
    if (output)
    {
      output->SetNumberOfPartitions(level, numDataSets);
      hierarchy->AddDataSetIndex(node, level);
      for (unsigned int cc = 0; cc < numDataSets; ++cc)
      {
        output->SetPartition(level, cc, amr->GetDataSet(level, cc));
      }
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkDataAssemblyUtilities::GenerateHierarchyInternal(
  vtkMultiBlockDataSet* input, vtkDataAssembly* hierarchy, vtkPartitionedDataSetCollection* output)
{
  assert(input != nullptr && hierarchy != nullptr);

  auto appendToOutput = [&](vtkDataObject* dobj, vtkInformation* metadata, int nodeid) {
    if (!output)
    {
      return;
    }

    const auto oid = output->GetNumberOfPartitionedDataSets();
    output->SetNumberOfPartitionedDataSets(oid + 1);
    hierarchy->AddDataSetIndex(nodeid, oid);
    if (metadata)
    {
      output->GetMetaData(oid)->Copy(metadata);
    }
    if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(dobj))
    {
      output->GetPartitionedDataSet(oid)->ShallowCopy(pdc);
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
    hierarchy->SetAttribute(nodeid, "cid", cid++);
    if (auto mb = vtkMultiBlockDataSet::SafeDownCast(dobj))
    {
      hierarchy->SetAttribute(nodeid, "vtk_type", dobj->GetDataObjectType());
      for (unsigned int bidx = 0, numBlocks = mb->GetNumberOfBlocks(); bidx < numBlocks; ++bidx)
      {
        auto metadata = mb->HasMetaData(bidx) ? mb->GetMetaData(bidx) : nullptr;
        auto child = hierarchy->AddNode(("Block" + std::to_string(bidx)).c_str(), nodeid);
        if (metadata && metadata->Has(vtkCompositeDataSet::NAME()))
        {
          hierarchy->SetAttribute(child, "label", metadata->Get(vtkCompositeDataSet::NAME()));
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
  hierarchy->SetAttribute(vtkDataAssembly::GetRootNode(), "vtk_category", "hierarchy");
  f(input, vtkDataAssembly::GetRootNode(), nullptr);
  return true;
}

//----------------------------------------------------------------------------
bool vtkDataAssemblyUtilities::GenerateHierarchyInternal(vtkPartitionedDataSetCollection* input,
  vtkDataAssembly* hierarchy, vtkPartitionedDataSetCollection* output)
{
  assert(input != nullptr && hierarchy != nullptr);
  if (output)
  {
    output->ShallowCopy(input);
  }

  hierarchy->SetRootNodeName("Root");
  hierarchy->SetAttribute(vtkDataAssembly::GetRootNode(), "vtk_type", input->GetDataObjectType());
  hierarchy->SetAttribute(vtkDataAssembly::GetRootNode(), "vtk_category", "hierarchy");
  for (unsigned int p = 0; p < input->GetNumberOfPartitionedDataSets(); ++p)
  {
    auto node = hierarchy->AddNode(("Block" + std::to_string(p)).c_str());
    auto metadata = input->HasMetaData(p) ? input->GetMetaData(p) : nullptr;
    if (metadata && metadata->Has(vtkCompositeDataSet::NAME()))
    {
      hierarchy->SetAttribute(node, "label", metadata->Get(vtkCompositeDataSet::NAME()));
    }
    hierarchy->SetAttribute(node, "bid", p);
    if (output)
    {
      hierarchy->AddDataSetIndex(node, p);
    }
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
          top->SetBlock(index, this->Input->GetPartition(datasets[0], 0));
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

  if (strcmp(hierarchy->GetAttributeOrDefault(root, "vtk_category", ""), "hierarchy") != 0)
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
      !blocks_per_level.empty() ? &blocks_per_level[0] : nullptr);
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

  bool LeavesOnly = false;
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
      if (this->LeavesOnly && vtkDataObjectTypes::TypeIdIsA(type, VTK_COMPOSITE_DATA_SET))
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
std::vector<unsigned int> vtkDataAssemblyUtilities::GenerateCompositeIndicesFromSelectors(
  vtkDataAssembly* hierarchy, const std::vector<std::string>& selectors,
  bool leaf_nodes_only /*=false*/)
{
  if (hierarchy == nullptr || selectors.empty())
  {
    return {};
  }

  const auto root = vtkDataAssembly::GetRootNode();
  if (strcmp(hierarchy->GetAttributeOrDefault(root, "vtk_category", ""), "hierarchy") != 0)
  {
    vtkLogF(
      ERROR, "Input hierarchy not generated using `vtkDataAssemblyUtilities` is not supported!");
    return {};
  }

  const auto dataType = hierarchy->GetAttributeOrDefault(root, "vtk_type", -1);

  // we only support MBs. we could support AMR, but I don't see the point in
  // doing so.
  if (!vtkDataObjectTypes::TypeIdIsA(dataType, VTK_MULTIBLOCK_DATA_SET))
  {
    vtkLogF(ERROR, "Hierarchy does not represent a supported composite dataset type (%s)",
      vtkDataObjectTypes::GetClassNameFromTypeId(dataType));
    return {};
  }

  auto nodes = hierarchy->SelectNodes(selectors);
  if (nodes.empty())
  {
    return {};
  }

  vtkNew<vtkGenerateIdsVisitor> visitor;
  visitor->LeavesOnly = leaf_nodes_only;
  std::copy(nodes.begin(), nodes.end(),
    std::inserter(visitor->SelectedNodes, visitor->SelectedNodes.end()));
  hierarchy->Visit(visitor);
  return visitor->CompositeIndices;
}
