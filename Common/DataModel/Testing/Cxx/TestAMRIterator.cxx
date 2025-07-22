// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInformation.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkOverlappingAMR.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMRIterator.h"

#include <iostream>
#include <vector>

namespace
{
constexpr unsigned int TEST_AMR_EXPECTED[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 1, 2 } };

//------------------------------------------------------------------------------
void InitializeNonOverlappingAMR(vtkNonOverlappingAMR* amr)
{
  // Create and populate the Non Overlapping AMR dataset.
  // The dataset should look like
  // Level 0
  //   uniform grid
  // Level 1
  //   uniform grid
  //   uniform grid
  //   empty node
  std::vector<unsigned int> blocksPerLevel{ 1, 3 };
  amr->Initialize(blocksPerLevel);

  double origin[3] = { 0.0, 0.0, 0.0 };
  double spacing[3] = { 1.0, 1.0, 1.0 };
  int dims[3] = { 11, 11, 6 };

  vtkNew<vtkUniformGrid> ug1;
  // Geometry
  ug1->SetOrigin(origin);
  ug1->SetSpacing(spacing);
  ug1->SetDimensions(dims);

  amr->SetDataSet(0, 0, ug1);

  double origin2[3] = { 0.0, 0.0, 5.0 };
  double spacing2[3] = { 1.0, 0.5, 1.0 };

  vtkNew<vtkUniformGrid> ug2;
  // Geometry
  ug2->SetOrigin(origin2);
  ug2->SetSpacing(spacing2);
  ug2->SetDimensions(dims);

  amr->SetDataSet(1, 0, ug2);

  double origin3[3] = { 0.0, 5.0, 5.0 };

  vtkNew<vtkUniformGrid> ug3;
  // Geometry
  ug3->SetOrigin(origin3);
  ug3->SetSpacing(spacing2);
  ug3->SetDimensions(dims);

  amr->SetDataSet(1, 1, ug3);
}

//------------------------------------------------------------------------------
void InitializeOverlappingAMR(vtkOverlappingAMR* amr)
{
  // Create and populate the AMR dataset.
  // The dataset should look like
  // Level 0
  //   uniform grid, dimensions 11, 11, 11, AMR box (0, 0, 0) - (9, 9, 9)
  // Level 1 - refinement ratio : 2
  //   uniform grid, dimensions 11, 11, 11, AMR box (0, 0, 0) - (9, 9, 9)
  //   uniform grid, dimensions 11, 11, 11, AMR box (10, 10, 10) - (19, 19, 19)
  //   empty node

  std::vector<unsigned int> blocksPerLevel{ 1, 3 };
  amr->Initialize(blocksPerLevel);

  double origin[3] = { 0.0, 0.0, 0.0 };
  double spacing[3] = { 1.0, 1.0, 1.0 };
  int dims[3] = { 11, 11, 11 };

  // Origin should be set as soon as it is known
  amr->SetOrigin(origin);

  vtkNew<vtkUniformGrid> ug1;
  // Geometry
  ug1->SetOrigin(origin);
  ug1->SetSpacing(spacing);
  ug1->SetDimensions(dims);

  // Spacing should be set before AMRBox
  amr->SetSpacing(0, spacing);

  int lo[3] = { 0, 0, 0 };
  int hi[3] = { 9, 9, 9 };
  vtkAMRBox box1(lo, hi);
  amr->SetAMRBox(0, 0, box1);
  amr->SetDataSet(0, 0, ug1);

  double spacing2[3] = { 0.5, 0.5, 0.5 };

  vtkNew<vtkUniformGrid> ug2;
  // Geometry
  ug2->SetOrigin(origin);
  ug2->SetSpacing(spacing2);
  ug2->SetDimensions(dims);

  // Spacing should be set before AMRBox
  amr->SetSpacing(1, spacing2);

  int lo2[3] = { 0, 0, 0 };
  int hi2[3] = { 9, 9, 9 };
  vtkAMRBox box2(lo2, hi2);
  amr->SetAMRBox(1, 0, box2);
  amr->SetDataSet(1, 0, ug2);

  double origin3[3] = { 5, 5, 5 };

  vtkNew<vtkUniformGrid> ug3;
  // Geometry
  ug3->SetOrigin(origin3);
  ug3->SetSpacing(spacing2);
  ug3->SetDimensions(dims);

  int lo3[3] = { 10, 10, 10 };
  int hi3[3] = { 19, 19, 19 };
  vtkAMRBox box3(lo3, hi3);
  amr->SetAMRBox(1, 1, box3);
  amr->SetDataSet(1, 1, ug3);

  amr->SetRefinementRatio(0, 2);
}

//------------------------------------------------------------------------------
bool LoopAndCheck(vtkUniformGridAMRIterator* iter, bool checkBounds = false)
{
  bool ret = true;
  iter->InitTraversal();
  for (int idx = 0; !iter->IsDoneWithTraversal(); iter->GoToNextItem(), ++idx)
  {
    unsigned int level = iter->GetCurrentLevel();
    unsigned int id = iter->GetCurrentIndex();
    if (level != TEST_AMR_EXPECTED[idx][0])
    {
      std::cerr << "Unexpected level, got: " << level << " expected: " << TEST_AMR_EXPECTED[idx][0]
                << std::endl;
      ret = false;
    }
    if (id != TEST_AMR_EXPECTED[idx][1])
    {
      std::cerr << "Unexpected id, got: " << id << " expected: " << TEST_AMR_EXPECTED[idx][1]
                << std::endl;
      ret = false;
    }

    if (checkBounds && !iter->GetCurrentMetaData()->Get(vtkDataObject::BOUNDING_BOX()))
    {
      std::cerr << "Failed to retrieve bounds" << std::endl;
      ret = false;
    }
  }
  return ret;
}
}

//------------------------------------------------------------------------------
int TestAMRIterator(int, char*[])
{
  bool success = true;

  vtkNew<vtkOverlappingAMR> oamr;
  ::InitializeOverlappingAMR(oamr);

  vtkSmartPointer<vtkUniformGridAMRIterator> oamrIter =
    vtkSmartPointer<vtkUniformGridAMRIterator>::Take(
      vtkUniformGridAMRIterator::SafeDownCast(oamr->NewIterator()));
  success &= ::LoopAndCheck(oamrIter, true);
  oamrIter->SkipEmptyNodesOff();
  success &= ::LoopAndCheck(oamrIter, true);

  vtkNew<vtkNonOverlappingAMR> noamr;
  ::InitializeNonOverlappingAMR(noamr);

  vtkSmartPointer<vtkUniformGridAMRIterator> noamrIter =
    vtkSmartPointer<vtkUniformGridAMRIterator>::Take(
      vtkUniformGridAMRIterator::SafeDownCast(noamr->NewIterator()));
  success &= ::LoopAndCheck(noamrIter);
  noamrIter->SkipEmptyNodesOff();
  success &= ::LoopAndCheck(noamrIter);

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
