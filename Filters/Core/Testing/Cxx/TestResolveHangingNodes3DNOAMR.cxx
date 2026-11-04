// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkResolveHangingNodes on a 3D vtkNonOverlappingAMR of vtkImageData blocks with 2 levels:
// polyhedron and decomposition modes, the output vtkPartitionedDataSetCollection of
// vtkUnstructuredGrid, a block only receiving the points of the blocks of the other levels, and a
// vtkImageData input.
#include "ResolveHangingNodesTestUtilities.h"

#include "vtkCellType.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkResolveHangingNodes.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <iostream>
#include <string>
#include <vector>

namespace
{
// The domain [0,6] x [0,2] x [0,2] of the AMR.
constexpr Domain AMR_DOMAIN = { 24.0, 56.0, 10 };

// The numbers of blocks of the levels of the AMR.
const std::vector<unsigned int> BLOCKS_PER_LEVEL = { 1, 2 };

// 2-level AMR over the domain [0,6] x [0,2] x [0,2]:
// - Level 0: [0,4] x [0,2] x [0,2], 2 x 1 x 1 cells of size 2. The face x = 4 of its cell 1 has
//   the hanging points of level 1: (4,1,0), (4,0,1), (4,2,1) and (4,1,2) on its edges, and
//   (4,1,1) at its center.
// - Level 1: [4,6] x [0,2] x [0,1] and [4,6] x [0,2] x [1,2], 2 x 2 x 1 cells of size 1 each,
//   which are conforming.
vtkSmartPointer<vtkNonOverlappingAMR> CreateAMR()
{
  auto amr = vtkSmartPointer<vtkNonOverlappingAMR>::New();
  amr->Initialize(BLOCKS_PER_LEVEL);
  amr->SetDataSet(0, 0, CreateBlock(0, { 0, 0, 0 }, { 2, 1, 1 }, 2.0, 0));
  amr->SetDataSet(1, 0, CreateBlock(1, { 4, 0, 0 }, { 2, 2, 1 }, 1.0, 2));
  amr->SetDataSet(1, 1, CreateBlock(1, { 4, 0, 1 }, { 2, 2, 1 }, 1.0, 6));
  return amr;
}

//------------------------------------------------------------------------------
// Polyhedron and decomposition modes: the cell 1 of level 0 gets the 5 points of its face x = 4,
// and becomes a polyhedron of 13 points, or 5 pyramids and 4 tetrahedra, without new point.
bool TestMode(bool decompose)
{
  const std::string name = decompose ? "DecompositionMode" : "PolyhedronMode";
  vtkNew<vtkLevelsTestFilter> filter;
  filter->SetInputData(CreateAMR());
  filter->SetDecompose(decompose);
  filter->SetComputeInverseMeasure(true);
  filter->Update();

  if (filter->GetNumberOfLevels() != 2)
  {
    std::cerr << name << ": expected 2 levels, got " << filter->GetNumberOfLevels() << "\n";
    return false;
  }
  if (filter->PartitionedDataSetIds != std::vector<int>{ 0, 1, 1 } ||
    !filter->ReceivedOnlyOtherLevels)
  {
    std::cerr << name << ": a block should only receive the points of the other levels\n";
    return false;
  }

  // The output is a vtkPartitionedDataSetCollection of vtkUnstructuredGrid, with the structure of
  // the AMR. Only the block of level 0 gets hanging points.
  auto output = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));
  if (!output || output->GetNumberOfPartitionedDataSets() != BLOCKS_PER_LEVEL.size())
  {
    std::cerr << name << ": the output should be a vtkPartitionedDataSetCollection of 2 levels\n";
    return false;
  }
  const std::array<vtkIdType, 3> numPoints = { 12 + 5, 18, 18 };
  std::size_t b = 0;
  for (unsigned int level = 0; level < BLOCKS_PER_LEVEL.size(); ++level)
  {
    if (output->GetNumberOfPartitions(level) != BLOCKS_PER_LEVEL[level])
    {
      std::cerr << name << ": level " << level << " has a wrong number of blocks\n";
      return false;
    }
    for (unsigned int i = 0; i < BLOCKS_PER_LEVEL[level]; ++i, ++b)
    {
      auto block = vtkUnstructuredGrid::SafeDownCast(output->GetPartition(level, i));
      if (!block || block->GetNumberOfPoints() != numPoints[b])
      {
        std::cerr << name << ": block " << i << " of level " << level
                  << " should be a vtkUnstructuredGrid with " << numPoints[b] << " points\n";
        return false;
      }
    }
  }
  // The hanging point (4,1,1) of level 1 is copied with its point data.
  vtkDataSet* coarse = output->GetPartition(0, 0);
  const vtkIdType id = FindPoint(coarse, { 4, 1, 1 });
  vtkDataArray* values = coarse->GetPointData()->GetArray("Values");
  vtkDataArray* levels = coarse->GetPointData()->GetArray("Levels");
  if (id < 0 || !values || values->GetTuple1(id) != 114.0 || !levels || levels->GetTuple1(id) != 1)
  {
    std::cerr << name << ": the hanging point of level 1 or its data is wrong\n";
    return false;
  }
  auto merged = Merge(output);
  if (!CheckOutput<3>(name, merged, decompose, 35, decompose ? 1 + 9 + 8 : 10, AMR_DOMAIN))
  {
    return false;
  }
  if (!decompose && (merged->GetCellType(1) != VTK_POLYHEDRON || merged->GetCellSize(1) != 13))
  {
    std::cerr << name << ": cell 1 should be a polyhedron with 13 points\n";
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
// A vtkImageData is conforming: it gives the vtkUnstructuredGrid of its cells, as hexahedra, with
// 1 level.
bool TestImageDataInput()
{
  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(CreateBlock(0, { 0, 0, 0 }, { 2, 2, 2 }, 1.0, 0));
  filter->SetComputeInverseMeasure(true);
  filter->Update();
  auto output = vtkUnstructuredGrid::SafeDownCast(filter->GetOutputDataObject(0));
  if (!output || filter->GetNumberOfLevels() != 1)
  {
    std::cerr << "ImageDataInput: the output should be a vtkUnstructuredGrid with 1 level\n";
    return false;
  }
  return CheckOutput<3>("ImageDataInput", output, true, 27, 8, { 8.0, 24.0, 8 });
}
} // namespace

int TestResolveHangingNodes3DNOAMR(int, char*[])
{
  if (BoundaryMeasure<3>(Merge(CreateAMR())) < AMR_DOMAIN.BoundaryMeasure + 1.0)
  {
    std::cerr << "The input should have hanging nodes\n";
    return EXIT_FAILURE;
  }
  if (!TestMode(false) || !TestMode(true) || !TestImageDataInput())
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
