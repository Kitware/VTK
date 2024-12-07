// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSphereSource.h"

#include <functional>
#include <map>
#include <string>

namespace
{

vtkSmartPointer<vtkPartitionedDataSet> CreatePartitionedDataSet(int count)
{
  vtkNew<vtkPartitionedDataSet> parts;
  parts->SetNumberOfPartitions(count);
  for (int cc = 0; cc < count; ++cc)
  {
    vtkNew<vtkSphereSource> sphere;
    sphere->SetCenter(cc, 0, 0);
    sphere->Update();
    parts->SetPartition(cc, sphere->GetOutputDataObject(0));
  }
  return parts;
}

vtkSmartPointer<vtkMultiPieceDataSet> CreateMultiPieceDataSet(int count)
{
  vtkNew<vtkMultiPieceDataSet> parts;
  parts->SetNumberOfPieces(count);
  for (int cc = 0; cc < count; ++cc)
  {
    vtkNew<vtkSphereSource> sphere;
    sphere->SetCenter(cc, 0, 0);
    sphere->Update();
    parts->SetPiece(cc, sphere->GetOutputDataObject(0));
  }
  return parts;
}

vtkSmartPointer<vtkDataObject> CreateDataSet()
{
  vtkNew<vtkSphereSource> sphere;
  sphere->Update();
  return sphere->GetOutputDataObject(0);
}

bool TestPartitionedDataSet()
{
  auto parts = CreatePartitionedDataSet(3);

  vtkNew<vtkDataAssembly> hierarchy;
  if (vtkDataAssemblyUtilities::GenerateHierarchy(parts, hierarchy))
  {
    vtkLogF(ERROR, "vtkDataAssemblyUtilities::GenerateHierarchy should return false.");
    return false;
  }
  return true;
}

bool TestMultiPieceDataSet()
{
  auto mp = CreateMultiPieceDataSet(3);
  vtkNew<vtkDataAssembly> hierarchy;
  if (vtkDataAssemblyUtilities::GenerateHierarchy(mp, hierarchy))
  {
    vtkLogF(ERROR, "vtkDataAssemblyUtilities::GenerateHierarchy should return false.");
    return false;
  }
  return true;
}

bool TestPartitionedDataSetCollection()
{
  vtkNew<vtkPartitionedDataSetCollection> collection;
  for (int cc = 0; cc < 5; ++cc)
  {
    collection->SetPartitionedDataSet(cc, CreatePartitionedDataSet(3));
  }

  vtkNew<vtkDataAssembly> hierarchy;
  if (!vtkDataAssemblyUtilities::GenerateHierarchy(collection, hierarchy))
  {
    vtkLogF(ERROR, "vtkDataAssemblyUtilities::GenerateHierarchy should return true.");
    return false;
  }
  hierarchy->Print(cout);

  const auto root = vtkDataAssembly::GetRootNode();
  vtkLogIfF(ERROR,
    hierarchy->GetAttributeOrDefault(root, "vtk_type", -1) != VTK_PARTITIONED_DATA_SET_COLLECTION,
    "vtk_type mismatch!");
  vtkLogIfF(ERROR, hierarchy->GetNumberOfChildren(root) != 5, "child count mismatch!");
  // 6, since we add cid for root and the 5 partitioned datasets.
  vtkLogIfF(ERROR, hierarchy->GetDataSetIndices(root).size() != 6, "dataset count mismatch!");

  vtkNew<vtkPartitionedDataSetCollection> xformed;
  if (!vtkDataAssemblyUtilities::GenerateHierarchy(collection, hierarchy, xformed))
  {
    vtkLogF(ERROR, "vtkDataAssemblyUtilities::GenerateHierarchy should return true.");
    return false;
  }

  vtkLogIfF(ERROR, xformed->GetDataAssembly()->GetDataSetIndices(root).size() != 5,
    "dataset count mismatch!");
  vtkLogIfF(
    ERROR, xformed->GetNumberOfPartitionedDataSets() != 5, "partitioned dataset count mismatch!");

  vtkLogIfF(ERROR,
    vtkDataAssemblyUtilities::GetSelectorForCompositeId(13u, hierarchy) != "/Root/Block3",
    "GetSelectorForCompositeId with id for non-leaf node failed!");
  vtkLogIfF(ERROR,
    vtkDataAssemblyUtilities::GetSelectorForCompositeId(10u, hierarchy) != "/Root/Block2",
    "GetSelectorForCompositeId with  id for leaf node failed!");
  return true;
}

vtkSmartPointer<vtkMultiBlockDataSet> CreateMultiBlock(
  const std::map<std::string, std::vector<std::string>>& map, int numPieces = 0)
{
  std::function<vtkSmartPointer<vtkDataObject>(const std::string&)> populate;
  populate = [&](const std::string& name) -> vtkSmartPointer<vtkDataObject>
  {
    auto iter = map.find(name);
    if (iter == map.end())
    {
      if (numPieces == 0)
      {
        return CreateDataSet();
      }
      else
      {
        return CreateMultiPieceDataSet(numPieces);
      }
    }

    const auto& blockNames = iter->second;
    vtkNew<vtkMultiBlockDataSet> mb;
    unsigned int numBlocks = static_cast<unsigned int>(blockNames.size());
    mb->SetNumberOfBlocks(numBlocks);
    for (unsigned int cc = 0; cc < numBlocks; ++cc)
    {
      mb->GetMetaData(cc)->Set(vtkCompositeDataSet::NAME(), blockNames[cc].c_str());
      mb->SetBlock(cc, populate(blockNames[cc]));
    }
    return mb;
  };

  return vtkMultiBlockDataSet::SafeDownCast(populate("Root"));
}

bool TestMultiBlockDataSet(int numPieces)
{
  vtkLogScopeF(INFO, "TestMultiBlockDataSet(%d)", numPieces);

  // this intentionally looks like an Exodus II reader output.
  // with one empty node name for better testing
  std::map<std::string, std::vector<std::string>> map = {
    { "Root", { "Element Blocks", "Face Blocks", "Side Sets", "Node Sets" } },
    { "Element Blocks", { "Unnamed block ID: 1", "Unnamed block ID: 2" } }, { "Face Sets", {} },
    { "Side Sets", { "Unnamed set ID: 4" } }, { "Node Sets", { "Unnamed set ID: 1", "" } }
  };
  auto mb = CreateMultiBlock(map, numPieces);

  vtkNew<vtkDataAssembly> hierarchy;
  if (!vtkDataAssemblyUtilities::GenerateHierarchy(mb, hierarchy))
  {
    vtkLogF(ERROR, "vtkDataAssemblyUtilities::GenerateHierarchy should return true.");
    return false;
  }
  hierarchy->Print(cout);

  auto XPath = [&hierarchy](const std::string& path)
  {
    auto nodes = hierarchy->SelectNodes({ path });
    return nodes.size() == 1 ? nodes[0] : -1;
  };

  const auto root = vtkDataAssembly::GetRootNode();
  vtkLogIfF(ERROR,
    hierarchy->GetAttributeOrDefault(root, "vtk_type", -1) != VTK_MULTIBLOCK_DATA_SET,
    "vtk_type mismatch");
  vtkLogIfF(ERROR,
    strcmp(hierarchy->GetAttributeOrDefault(XPath("//*[@cid=2]"), "label", ""),
      "Unnamed block ID: 1") != 0,
    "label mismatch");
  vtkLogIfF(ERROR, hierarchy->GetNumberOfChildren(XPath("//*[@label='Node Sets']")) != 2,
    "node sets mismatch");

  if (numPieces == 0)
  {
    vtkLogIfF(ERROR,
      hierarchy->GetDataSetIndices(7, /*traverse_subtree=*/false) !=
        std::vector<unsigned int>({ 7u }),
      "GetDataSetIndices incorrect.");

    vtkLogIfF(ERROR,
      hierarchy->GetDataSetIndices(7, /*traverse_subtree=*/true) !=
        std::vector<unsigned int>({ 7u, 8u, 9u }),
      "GetDataSetIndices incorrect.");

    vtkLogIfF(ERROR,
      vtkDataAssemblyUtilities::GetSelectedCompositeIds(
        { "//*[@label='Node Sets']" }, hierarchy, nullptr) != std::vector<unsigned int>{ 7 },
      "node set cid mismatch");

    vtkLogIfF(ERROR,
      vtkDataAssemblyUtilities::GetSelectorForCompositeId(3, hierarchy) !=
        "/Root/ElementBlocks/UnnamedblockID2",
      "GetSelectedCompositeIds mismatch");
  }
  else if (numPieces == 2)
  {
    vtkLogIfF(ERROR,
      hierarchy->GetDataSetIndices(7, /*traverse_subtree=*/false) !=
        std::vector<unsigned int>({ 15u }),
      "GetDataSetIndices incorrect.");

    vtkLogIfF(ERROR,
      hierarchy->GetDataSetIndices(7, /*traverse_subtree=*/true) !=
        std::vector<unsigned int>({ 15u, 16u, 19u }),
      "GetDataSetIndices incorrect.");

    vtkLogIfF(ERROR,
      vtkDataAssemblyUtilities::GetSelectedCompositeIds(
        { "//*[@label='Node Sets']" }, hierarchy, nullptr) != std::vector<unsigned int>{ 15 },
      "node set cid mismatch");

    vtkLogIfF(ERROR,
      vtkDataAssemblyUtilities::GetSelectorForCompositeId(3, hierarchy) !=
        "/Root/ElementBlocks/UnnamedblockID1",
      "GetSelectedCompositeIds mismatch");
  }

  vtkNew<vtkPartitionedDataSetCollection> xformed;
  if (!vtkDataAssemblyUtilities::GenerateHierarchy(mb, hierarchy, xformed))
  {
    vtkLogF(ERROR, "vtkDataAssemblyUtilities::GenerateHierarchy should return true.");
    return false;
  }

  vtkLogIfF(ERROR, xformed->GetNumberOfPartitionedDataSets() != 6, "dataset source mismatch");
  return true;
}

} // end of namespace

int TestDataAssemblyUtilities(int, char*[])
{
  if (!TestPartitionedDataSet() || !TestMultiPieceDataSet() ||
    !TestPartitionedDataSetCollection() || !TestMultiBlockDataSet(0) || !TestMultiBlockDataSet(2))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
