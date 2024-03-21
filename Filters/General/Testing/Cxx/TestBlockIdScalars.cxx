// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBlockIdScalars.h"
#include "vtkCellData.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkUnsignedCharArray.h"

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

  vtkMultiBlockDataSet* outputMb1 = vtkMultiBlockDataSet::SafeDownCast(blockIdFilter->GetOutput());
  vtkMultiBlockDataSet* outputMb0 = vtkMultiBlockDataSet::SafeDownCast(outputMb1->GetBlock(0));
  std::array<vtkImageData*, 3> outputImages{ vtkImageData::SafeDownCast(outputMb0->GetBlock(0)),
    vtkImageData::SafeDownCast(outputMb0->GetBlock(1)),
    vtkImageData::SafeDownCast(outputMb1->GetBlock(1)) };

  for (int leafId = 0; leafId < static_cast<int>(outputImages.size()); leafId++)
  {
    vtkDataArray* blockIdArray = outputImages[leafId]->GetCellData()->GetArray("BlockIdScalars");
    unsigned char expectedBlockIdValue = 0;
    if (leafId == 2)
    {
      expectedBlockIdValue = 1;
    }

    for (int cellId = 0; cellId < outputImages[leafId]->GetNumberOfCells(); cellId++)
    {
      if (blockIdArray->GetTuple1(cellId) != expectedBlockIdValue)
      {
        std::cerr << "Wrong BlockIdScalars value for cell " << cellId << " in leaf block " << leafId
                  << ". Got " << blockIdArray->GetTuple1(cellId) << " but expected "
                  << static_cast<int>(expectedBlockIdValue);
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
