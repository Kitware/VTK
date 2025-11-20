// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBlockIdScalars.h"
#include "vtkCellData.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"

#include <iostream>

namespace
{
bool CheckExpectedIds(vtkDataObjectTree* output, const std::array<int, 3>& expectedIds)
{
  auto iter = vtkSmartPointer<vtkDataObjectTreeIterator>::Take(output->NewTreeIterator());
  iter->TraverseSubTreeOn();
  iter->VisitOnlyLeavesOn();
  int leafId = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), leafId++)
  {
    vtkImageData* img = vtkImageData::SafeDownCast(iter->GetCurrentDataObject());
    vtkDataArray* blockIdArray = img->GetCellData()->GetArray("BlockIdScalars");
    double* range = blockIdArray->GetRange();
    if (range[0] != expectedIds[leafId] || range[1] != expectedIds[leafId])
    {
      std::cerr << "Wrong BlockIdScalars range for leaf " << leafId << ". Got [" << range[0] << ","
                << range[1] << "] instead of " << expectedIds[leafId] << std::endl;
      return false;
    }
  }
  return true;
}
}
int TestBlockIdScalars(int, char*[])
{
  // Create multiblock recursive structure
  vtkNew<vtkImageData> im0;
  im0->SetDimensions(3, 3, 3);
  vtkNew<vtkImageData> im1;
  im1->SetDimensions(2, 2, 2);
  vtkNew<vtkImageData> im2;
  im2->SetDimensions(4, 4, 4);

  vtkNew<vtkMultiBlockDataSet> mb0;
  mb0->SetNumberOfBlocks(2);
  mb0->SetBlock(0, im0);
  mb0->SetBlock(1, im1);

  vtkNew<vtkMultiBlockDataSet> mb1;
  mb1->SetNumberOfBlocks(2);
  mb1->SetBlock(0, mb0);
  mb1->SetBlock(1, im2);

  vtkNew<vtkBlockIdScalars> blockIdFilter;
  blockIdFilter->SetInputData(mb1);
  blockIdFilter->Update();

  vtkDataObjectTree* output = vtkDataObjectTree::SafeDownCast(blockIdFilter->GetOutput());
  std::array<int, 3> expectedIds{ 0, 0, 1 };
  if (!::CheckExpectedIds(output, expectedIds))
  {
    return EXIT_FAILURE;
  }

  blockIdFilter->TraverseSubTreeOn();
  blockIdFilter->VisitOnlyLeavesOn();
  blockIdFilter->Update();

  output = vtkDataObjectTree::SafeDownCast(blockIdFilter->GetOutput());
  expectedIds = { 0, 1, 2 };
  if (!::CheckExpectedIds(output, expectedIds))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
