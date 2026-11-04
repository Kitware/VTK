// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkResolveHangingNodes on a vtkPartitionedDataSetCollection of a 3D mesh of hexahedra
// with 2 levels: the output has the structure of the input, with null partitions and partitions of
// different types, the partitions of a vtkPartitionedDataSet are resolved together, and every
// vtkPartitionedDataSet on its own, and empty inputs.
#include "ResolveHangingNodesTestUtilities.h"

#include "vtkCellType.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkResolveHangingNodes.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <cmath>
#include <iostream>

namespace
{
//------------------------------------------------------------------------------
// A vtkPartitionedDataSetCollection of 2 vtkPartitionedDataSet: the first one has the
// vtkUnstructuredGrid cells A, B and C of the 2-level mesh, a null partition, and its cells D in a
// vtkImageData, which are resolved together, and the second one has the whole vtkUnstructuredGrid
// mesh, resolved on its own. The output has the structure of the input, and its partitions are
// vtkUnstructuredGrid.
bool TestStructure()
{
  auto mesh = CreateTwoLevelMesh();
  vtkNew<vtkPartitionedDataSet> first;
  first->SetPartition(0, ExtractCells(mesh, 0, TWO_LEVEL_SPLIT));
  first->SetPartition(1, nullptr);
  // The partitions exchange their points only if they have the same point data arrays.
  auto block = CreateBlock(1, { 2, 0, 0 }, { 2, 2, 2 }, 1.0, TWO_LEVEL_SPLIT);
  block->GetPointData()->RemoveArray("Levels");
  first->SetPartition(2, block);
  vtkNew<vtkPartitionedDataSet> second;
  second->SetPartition(0, mesh);
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
    !vtkUnstructuredGrid::SafeDownCast(outputFirst->GetPartition(0)) ||
    outputFirst->GetPartition(1) ||
    !vtkUnstructuredGrid::SafeDownCast(outputFirst->GetPartition(2)) ||
    !vtkUnstructuredGrid::SafeDownCast(outputSecond->GetPartition(0)))
  {
    std::cerr << "Structure: the output should have the structure of the input\n";
    return false;
  }
  return CheckOutput<3>("Structure(first)", Merge(outputFirst), false, 37, 11, TWO_LEVEL_DOMAIN) &&
    CheckOutput<3>("Structure(second)", Merge(outputSecond), false, 37, 11, TWO_LEVEL_DOMAIN);
}

//------------------------------------------------------------------------------
// The cells A and D of the 2-level mesh, in 2 partitions, and the cells B and C in 2
// vtkPartitionedDataSet, each resolved on its own: the hanging points of the face x = 2 of A are
// in its vtkPartitionedDataSet, so they are inserted, and A and D are conforming, of boundary area
// 40, but the hanging points of B and C are in the other one, so they are not.
bool TestSeparatePartitionedDataSets()
{
  auto mesh = CreateTwoLevelMesh();
  vtkNew<vtkPartitionedDataSet> first;
  first->SetPartition(0, ExtractCells(mesh, 0, 1));
  first->SetPartition(1, ExtractCells(mesh, TWO_LEVEL_SPLIT, mesh->GetNumberOfCells()));
  vtkNew<vtkPartitionedDataSet> second;
  second->SetPartition(0, ExtractCells(mesh, 1, TWO_LEVEL_SPLIT));
  vtkNew<vtkPartitionedDataSetCollection> input;
  input->SetPartitionedDataSet(0, first);
  input->SetPartitionedDataSet(1, second);

  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(input);
  filter->SetDecompose(false);
  filter->Update();

  auto output = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));
  vtkDataSet* outputA = output ? output->GetPartition(0, 0) : nullptr;
  vtkDataSet* outputD = output ? output->GetPartition(0, 1) : nullptr;
  vtkDataSet* outputBC = output ? output->GetPartition(1, 0) : nullptr;
  if (!outputA || !outputD || !outputBC || outputA->GetNumberOfPoints() != 13 ||
    outputA->GetNumberOfCells() != 1 || outputD->GetNumberOfPoints() != 27 ||
    outputD->GetNumberOfCells() != 8 || outputBC->GetNumberOfPoints() != 12 ||
    outputBC->GetNumberOfCells() != 2)
  {
    std::cerr << "SeparatePartitionedDataSets: wrong numbers of points or cells\n";
    return false;
  }
  if (outputA->GetCellType(0) != VTK_POLYHEDRON || outputA->GetCellSize(0) != 13 ||
    outputBC->GetCellType(0) != VTK_HEXAHEDRON || outputBC->GetCellType(1) != VTK_HEXAHEDRON)
  {
    std::cerr
      << "SeparatePartitionedDataSets: A should be a polyhedron with 13 points, and B and C "
         "unchanged\n";
    return false;
  }
  if (std::abs(BoundaryMeasure<3>(Merge(output->GetPartitionedDataSet(0))) - 40.0) > EPSILON)
  {
    std::cerr << "SeparatePartitionedDataSets: A and D should be conforming\n";
    return false;
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
      input->SetPartition(1, i, vtkSmartPointer<vtkUnstructuredGrid>::New());
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
      success = success && vtkUnstructuredGrid::SafeDownCast(partition) &&
        partition->GetNumberOfCells() == 0;
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

int TestResolveHangingNodes3DPDC(int, char*[])
{
  if (!TestStructure() || !TestSeparatePartitionedDataSets() || !TestEmptyInput())
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
