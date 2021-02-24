/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataAssemblyUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  auto parts = CreatePartitionedDataSet(3);
  vtkNew<vtkMultiPieceDataSet> mp;
  mp->ShallowCopy(parts);

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

  const auto root = vtkDataAssembly::GetRootNode();
  vtkLogIfF(ERROR,
    hierarchy->GetAttributeOrDefault(root, "vtk_type", -1) != VTK_PARTITIONED_DATA_SET_COLLECTION,
    "vtk_type mismatch!");
  vtkLogIfF(ERROR, hierarchy->GetNumberOfChildren(root) != 5, "child count mismatch!");
  vtkLogIfF(ERROR, !hierarchy->GetDataSetIndices(root).empty(), "dataset count mismatch!");

  vtkNew<vtkPartitionedDataSetCollection> xformed;
  if (!vtkDataAssemblyUtilities::GenerateHierarchy(collection, hierarchy, xformed))
  {
    vtkLogF(ERROR, "vtkDataAssemblyUtilities::GenerateHierarchy should return true.");
    return false;
  }

  vtkLogIfF(ERROR, hierarchy->GetDataSetIndices(root).size() != 5, "dataset count mismatch!");
  vtkLogIfF(
    ERROR, xformed->GetNumberOfPartitionedDataSets() != 5, "partitioned dataset count mismatch!");

  return true;
}

vtkSmartPointer<vtkMultiBlockDataSet> CreateMultiBlock(
  const std::map<std::string, std::vector<std::string>>& map)
{
  std::function<vtkSmartPointer<vtkDataObject>(const std::string&)> populate;
  populate = [&](const std::string& name) -> vtkSmartPointer<vtkDataObject> {
    auto iter = map.find(name);
    if (iter == map.end())
    {
      return CreateDataSet();
    }

    const auto& blockNames = iter->second;
    vtkNew<vtkMultiBlockDataSet> mb;
    mb->SetNumberOfBlocks(blockNames.size());
    for (size_t cc = 0; cc < blockNames.size(); ++cc)
    {
      mb->GetMetaData(cc)->Set(vtkCompositeDataSet::NAME(), blockNames[cc].c_str());
      mb->SetBlock(cc, populate(blockNames[cc]));
    }
    return mb;
  };

  return vtkMultiBlockDataSet::SafeDownCast(populate("Root"));
}

bool TestMultiBlockDataSet()
{
  // this intentionally looks like an Exodus II reader output.
  std::map<std::string, std::vector<std::string>> map = {
    { "Root", { "Element Blocks", "Face Blocks", "Side Sets", "Node Sets" } },
    { "Element Blocks", { "Unnamed block ID: 1", "Unnamed block ID: 2" } }, { "Face Sets", {} },
    { "Side Sets", { "Unnamed set ID: 4" } },
    { "Node Sets", { "Unnamed set ID: 1", "Unnamed set ID: 100" } }
  };
  auto mb = CreateMultiBlock(map);

  vtkNew<vtkDataAssembly> hierarchy;
  if (!vtkDataAssemblyUtilities::GenerateHierarchy(mb, hierarchy))
  {
    vtkLogF(ERROR, "vtkDataAssemblyUtilities::GenerateHierarchy should return true.");
    return false;
  }

  auto XPath = [&hierarchy](const std::string& path) {
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

  vtkLogIfF(ERROR,
    vtkDataAssemblyUtilities::GenerateCompositeIndicesFromSelectors(
      hierarchy, { "//*[@label='Node Sets']" }) != std::vector<unsigned int>{ 7 },
    "node set cid mismatch");

  vtkNew<vtkPartitionedDataSetCollection> xformed;
  if (!vtkDataAssemblyUtilities::GenerateHierarchy(mb, hierarchy, xformed))
  {
    vtkLogF(ERROR, "vtkDataAssemblyUtilities::GenerateHierarchy should return true.");
    return false;
  }

  hierarchy->Print(cout);
  vtkLogIfF(ERROR, xformed->GetNumberOfPartitionedDataSets() != 6, "dataset source mismatch");
  return true;
}

} // end of namespace

int TestDataAssemblyUtilities(int, char*[])
{
  if (!TestPartitionedDataSet() || !TestMultiPieceDataSet() ||
    !TestPartitionedDataSetCollection() || !TestMultiBlockDataSet())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
