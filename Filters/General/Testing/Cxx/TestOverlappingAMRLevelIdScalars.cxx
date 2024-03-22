// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkOverlappingAMRLevelIdScalars.h"
#include "vtkUniformGrid.h"

int TestOverlappingAMRLevelIdScalars(int, char*[])
{
  // Create overlapping AMR with 2 levels
  vtkNew<vtkOverlappingAMR> amr;
  std::array<int, 2> blocksPerLevel{ 2, 1 };
  amr->Initialize(static_cast<int>(blocksPerLevel.size()), blocksPerLevel.data());

  // Attach datasets to the AMR
  vtkNew<vtkUniformGrid> root;
  root->SetDimensions(3, 3, 3);
  root->SetSpacing(1.0, 1.0, 1.0);
  amr->SetDataSet(0, 0, root);
  amr->SetDataSet(0, 1, root);
  amr->SetDataSet(1, 0, root);

  // Apply id filter
  vtkNew<vtkOverlappingAMRLevelIdScalars> levelIdFilter;
  levelIdFilter->SetInputData(amr);
  levelIdFilter->Update();

  // Retrieve level datasets
  vtkOverlappingAMR* outputAMR = vtkOverlappingAMR::SafeDownCast(levelIdFilter->GetOutput());
  std::array<vtkUniformGrid*, 3> outputImages{ vtkUniformGrid::SafeDownCast(
                                                 outputAMR->GetDataSet(0, 0)),
    vtkUniformGrid::SafeDownCast(outputAMR->GetDataSet(0, 1)),
    vtkUniformGrid::SafeDownCast(outputAMR->GetDataSet(1, 0)) };

  // Check level id field
  for (int datasetId = 0; datasetId < static_cast<int>(outputImages.size()); datasetId++)
  {
    vtkDataArray* levelIdArray = outputImages[datasetId]->GetCellData()->GetArray("LevelIdScalars");
    unsigned char expectedLevelIdValue = 0;
    if (datasetId == 2)
    {
      expectedLevelIdValue = 1;
    }

    for (int cellId = 0; cellId < outputImages[datasetId]->GetNumberOfCells(); cellId++)
    {
      if (levelIdArray->GetTuple1(cellId) != expectedLevelIdValue)
      {
        std::cerr << "Wrong LevelIdScalars value for cell " << cellId << " in dataset " << datasetId
                  << ". Got " << levelIdArray->GetTuple1(cellId) << " but expected "
                  << static_cast<int>(expectedLevelIdValue);
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
