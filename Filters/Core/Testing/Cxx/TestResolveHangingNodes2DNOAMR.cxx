// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkResolveHangingNodes on a 2D vtkNonOverlappingAMR of vtkImageData blocks with 3 levels:
// polygon and decomposition modes, the output vtkPartitionedDataSetCollection of
// vtkUnstructuredGrid, a block only receiving the points of the blocks of the other levels, a
// vtkImageData input, and an AMR without blocks.
#include "ResolveHangingNodesTestUtilities.h"

#include "vtkCellType.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
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
// The domain [0,8] x [0,4] of the AMR.
constexpr Domain AMR_DOMAIN = { 32.0, 24.0, 32 };

// The numbers of blocks of the levels of the AMR.
const std::vector<unsigned int> BLOCKS_PER_LEVEL = { 1, 2, 1 };

// 3-level AMR over the domain [0,8] x [0,4]:
// - Level 0: [0,4] x [0,4], 2 x 2 cells of size 2. Its edge x = 4 has the hanging points (4,1)
//   and (4,3) of level 1.
// - Level 1: A = [4,6] x [0,4], 2 x 4 cells of size 1, whose edge x = 6 has the hanging points
//   (6,2.5) and (6,3.5) of level 2 for y > 2, and B = [6,8] x [0,2], 2 x 2 cells of size 1, whose
//   edge y = 2 has the hanging points (6.5,2) and (7.5,2) of level 2. A and B are conforming.
// - Level 2: [6,8] x [2,4], 4 x 4 cells of size 0.5.
vtkSmartPointer<vtkNonOverlappingAMR> CreateAMR()
{
  auto amr = vtkSmartPointer<vtkNonOverlappingAMR>::New();
  amr->Initialize(BLOCKS_PER_LEVEL);
  amr->SetDataSet(0, 0, CreateBlock(0, { 0, 0, 0 }, { 2, 2, 0 }, 2.0, 0));
  amr->SetDataSet(1, 0, CreateBlock(1, { 4, 0, 0 }, { 2, 4, 0 }, 1.0, 4));
  amr->SetDataSet(1, 1, CreateBlock(1, { 6, 0, 0 }, { 2, 2, 0 }, 1.0, 12));
  amr->SetDataSet(2, 0, CreateBlock(2, { 6, 2, 0 }, { 4, 4, 0 }, 0.5, 16));
  return amr;
}

//------------------------------------------------------------------------------
// Polygon and decomposition modes: the 2 cells of every block of the levels 0 and 1 along a finer
// block get 1 hanging point each, which becomes a 5-gon, or the fan of 3 triangles from it.
bool TestMode(bool decompose)
{
  const std::string name = decompose ? "DecompositionMode" : "PolygonMode";
  vtkNew<vtkLevelsTestFilter> filter;
  filter->SetInputData(CreateAMR());
  filter->SetDecompose(decompose);
  filter->SetComputeInverseMeasure(true);
  filter->Update();

  if (filter->GetNumberOfLevels() != 3)
  {
    std::cerr << name << ": expected 3 levels, got " << filter->GetNumberOfLevels() << "\n";
    return false;
  }
  if (filter->PartitionedDataSetIds != std::vector<int>{ 0, 1, 1, 2 } ||
    !filter->ReceivedOnlyOtherLevels)
  {
    std::cerr << name << ": a block should only receive the points of the other levels\n";
    return false;
  }

  // The output is a vtkPartitionedDataSetCollection of vtkUnstructuredGrid, with the structure of
  // the AMR. The blocks get their hanging points.
  auto output = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));
  if (!output || output->GetNumberOfPartitionedDataSets() != BLOCKS_PER_LEVEL.size())
  {
    std::cerr << name << ": the output should be a vtkPartitionedDataSetCollection of 3 levels\n";
    return false;
  }
  const std::array<vtkIdType, 4> numPoints = { 9 + 2, 15 + 2, 9 + 2, 25 };
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
  // The hanging point (4,1) of level 1 is copied with its point data.
  vtkDataSet* coarse = output->GetPartition(0, 0);
  const vtkIdType id = FindPoint(coarse, { 4, 1, 0 });
  vtkDataArray* values = coarse->GetPointData()->GetArray("Values");
  vtkDataArray* levels = coarse->GetPointData()->GetArray("Levels");
  if (id < 0 || !values || values->GetTuple1(id) != 14.0 || !levels || levels->GetTuple1(id) != 1)
  {
    std::cerr << name << ": the hanging point of level 1 or its data is wrong\n";
    return false;
  }
  return CheckOutput<2>(
    name, Merge(output), decompose, 47, decompose ? 32 + 6 * 2 : 32, AMR_DOMAIN);
}

//------------------------------------------------------------------------------
// A vtkImageData is conforming: it gives the vtkUnstructuredGrid of its cells, as quads, with 1
// level.
bool TestImageDataInput()
{
  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(CreateBlock(0, { 0, 0, 0 }, { 2, 2, 0 }, 1.0, 0));
  filter->SetComputeInverseMeasure(true);
  filter->Update();
  auto output = vtkUnstructuredGrid::SafeDownCast(filter->GetOutputDataObject(0));
  if (!output || filter->GetNumberOfLevels() != 1)
  {
    std::cerr << "ImageDataInput: the output should be a vtkUnstructuredGrid with 1 level\n";
    return false;
  }
  return CheckOutput<2>("ImageDataInput", output, true, 9, 4, { 4.0, 8.0, 4 });
}

//------------------------------------------------------------------------------
// An AMR without blocks gives a vtkPartitionedDataSetCollection of its structure, without
// partitions.
bool TestEmptyInput()
{
  vtkNew<vtkNonOverlappingAMR> input;
  input->Initialize(BLOCKS_PER_LEVEL);
  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(input);
  filter->Update();
  auto output = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));
  if (!output || output->GetNumberOfPartitionedDataSets() != BLOCKS_PER_LEVEL.size() ||
    !vtkCompositeDataSet::GetDataSets(output).empty())
  {
    std::cerr << "EmptyInput: the output should have the structure of the input, without "
                 "partitions\n";
    return false;
  }
  return true;
}
} // namespace

int TestResolveHangingNodes2DNOAMR(int, char*[])
{
  if (BoundaryMeasure<2>(Merge(CreateAMR())) < AMR_DOMAIN.BoundaryMeasure + 1.0)
  {
    std::cerr << "The input should have hanging nodes\n";
    return EXIT_FAILURE;
  }
  if (!TestMode(false) || !TestMode(true) || !TestImageDataInput() || !TestEmptyInput())
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
