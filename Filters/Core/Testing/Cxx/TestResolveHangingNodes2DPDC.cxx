// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkResolveHangingNodes on a vtkPartitionedDataSetCollection of a 2D mesh of quads with 3
// levels: the output has the structure of the input, with null partitions and partitions of
// different types, the partitions of a vtkPartitionedDataSet are resolved together, and every
// vtkPartitionedDataSet on its own, and empty inputs.
#include "ResolveHangingNodesTestUtilities.h"

#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkResolveHangingNodes.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <iostream>

namespace
{
//------------------------------------------------------------------------------
// A vtkPartitionedDataSetCollection of 2 vtkPartitionedDataSet: the first one has the vtkPolyData
// cells 0-3 of the 3-level mesh, a null partition, and its vtkUnstructuredGrid cells 4-7, which are
// resolved together, and the second one has the whole vtkUnstructuredGrid mesh, resolved on its
// own. The output has the structure of the input, and its partitions the types of the input ones.
bool TestStructure()
{
  auto polyData = CreateThreeLevelMesh();
  auto unstructuredGrid = ToUnstructuredGrid(polyData);
  vtkNew<vtkPartitionedDataSet> first;
  first->SetPartition(0, ExtractCells(polyData, 0, THREE_LEVEL_SPLIT));
  first->SetPartition(1, nullptr);
  first->SetPartition(
    2, ExtractCells(unstructuredGrid, THREE_LEVEL_SPLIT, unstructuredGrid->GetNumberOfCells()));
  vtkNew<vtkPartitionedDataSet> second;
  second->SetPartition(0, unstructuredGrid);
  vtkNew<vtkPartitionedDataSetCollection> input;
  input->SetPartitionedDataSet(0, first);
  input->SetPartitionedDataSet(1, second);

  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(input);
  filter->SetDecompose(false);
  filter->SetComputeInverseMeasure(true);
  filter->Update();

  auto output = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));
  vtkPartitionedDataSet* outputFirst = output ? output->GetPartitionedDataSet(0) : nullptr;
  vtkPartitionedDataSet* outputSecond = output ? output->GetPartitionedDataSet(1) : nullptr;
  if (!outputFirst || !outputSecond || output->GetNumberOfPartitionedDataSets() != 2 ||
    outputFirst->GetNumberOfPartitions() != 3 || outputSecond->GetNumberOfPartitions() != 1 ||
    !vtkPolyData::SafeDownCast(outputFirst->GetPartition(0)) || outputFirst->GetPartition(1) ||
    !vtkUnstructuredGrid::SafeDownCast(outputFirst->GetPartition(2)) ||
    !vtkUnstructuredGrid::SafeDownCast(outputSecond->GetPartition(0)))
  {
    std::cerr << "Structure: the output should have the structure of the input\n";
    return false;
  }
  return CheckOutput<2>("Structure(first)", Merge(outputFirst), false, 16, 8, THREE_LEVEL_DOMAIN) &&
    CheckOutput<2>("Structure(second)", Merge(outputSecond), false, 16, 8, THREE_LEVEL_DOMAIN);
}

//------------------------------------------------------------------------------
// The cells 0-3 and 4-7 of the 3-level mesh in 2 vtkPartitionedDataSet, each resolved on its own:
// the hanging point (4,2) of cell 0 is in its vtkPartitionedDataSet, so it is inserted, but the
// hanging points (7,2) and (6,3) of the cells 2 and 3 are in the other one, so they are not.
bool TestSeparatePartitionedDataSets()
{
  auto input = Split(CreateThreeLevelMesh(), THREE_LEVEL_SPLIT);
  vtkPartitionedDataSet* partitions = input->GetPartitionedDataSet(0);
  vtkNew<vtkPartitionedDataSet> second;
  second->SetPartition(0, partitions->GetPartition(1));
  partitions->SetNumberOfPartitions(1);
  input->SetPartitionedDataSet(1, second);

  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(input);
  filter->SetDecompose(false);
  filter->Update();

  auto output = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));
  vtkDataSet* outputFirst = output ? output->GetPartition(0, 0) : nullptr;
  vtkDataSet* outputSecond = output ? output->GetPartition(1, 0) : nullptr;
  if (!outputFirst || !outputSecond || outputFirst->GetNumberOfPoints() != 10 ||
    outputFirst->GetNumberOfCells() != 4 || outputSecond->GetNumberOfPoints() != 9 ||
    outputSecond->GetNumberOfCells() != 4)
  {
    std::cerr << "SeparatePartitionedDataSets: wrong numbers of points or cells\n";
    return false;
  }
  const std::array<vtkIdType, 4> sizes = { 5, 4, 4, 4 };
  for (vtkIdType c = 0; c < 4; ++c)
  {
    if (outputFirst->GetCellSize(c) != sizes[c])
    {
      std::cerr << "SeparatePartitionedDataSets: cell " << c << " should have " << sizes[c]
                << " points\n";
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// A vtkPartitionedDataSetCollection without vtkPartitionedDataSet, or with a vtkPartitionedDataSet
// without partitions and one with empty partitions, gives an output of the same structure, with
// empty partitions.
bool TestEmptyInput()
{
  for (unsigned int numPartitionedDataSets : { 0u, 2u })
  {
    vtkNew<vtkPartitionedDataSetCollection> input;
    input->SetNumberOfPartitionedDataSets(numPartitionedDataSets);
    for (unsigned int i = 0; numPartitionedDataSets > 0 && i < 2; ++i)
    {
      input->SetPartition(1, i, vtkSmartPointer<vtkPolyData>::New());
    }
    vtkNew<vtkResolveHangingNodes> filter;
    filter->SetInputData(input);
    filter->Update();
    auto output = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));
    bool success = output && output->GetNumberOfPartitionedDataSets() == numPartitionedDataSets;
    for (unsigned int i = 0; success && i < numPartitionedDataSets; ++i)
    {
      success = output->GetNumberOfPartitions(i) == input->GetNumberOfPartitions(i);
    }
    for (vtkDataSet* partition : vtkCompositeDataSet::GetDataSets(output))
    {
      success =
        success && vtkPolyData::SafeDownCast(partition) && partition->GetNumberOfCells() == 0;
    }
    if (!success)
    {
      std::cerr << "EmptyInput: the output should have the structure of the input, with "
                   "empty partitions\n";
      return false;
    }
  }
  return true;
}
} // namespace

int TestResolveHangingNodes2DPDC(int, char*[])
{
  if (!TestStructure() || !TestSeparatePartitionedDataSets() || !TestEmptyInput())
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
