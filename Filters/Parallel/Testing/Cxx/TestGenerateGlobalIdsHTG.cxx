// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Tests vtkHyperTreeGridGenerateGlobalIds through the new GlobalIds dataset attribute.

#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGenerateGlobalIds.h"
#include "vtkIdTypeArray.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkRandomHyperTreeGridSource.h"

#include <array>

namespace
{
const std::array<int, 4> offsets = { 0, 776, 1552, 2328 };
}

static int TestGlobalIdsArray(vtkCellData* cellData, int rank);

int TestGenerateGlobalIdsHTG(int argc, char* argv[])
{
  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller);

  // Create and execute pipeline
  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetSeed(42);
  htgSource->SetMaxDepth(3);
  htgSource->SetDimensions(3, 3, 3);
  htgSource->SetSplitFraction(0.5);

  vtkNew<vtkHyperTreeGridGenerateGlobalIds> generateGlobalIds;
  generateGlobalIds->SetInputConnection(htgSource->GetOutputPort());
  generateGlobalIds->Update();

  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(generateGlobalIds->GetOutput());
  int myRank = controller->GetLocalProcessId();

  int retVal = TestGlobalIdsArray(output->GetCellData(), myRank);

  controller->Finalize();
  return retVal;
}

int TestGlobalIdsArray(vtkCellData* cellData, int rank)
{
  vtkDataArray* globalIdsArray = cellData->GetGlobalIds();
  vtkIdType nbTuples = cellData->GetNumberOfTuples();

  if (!globalIdsArray)
  {
    vtkGenericWarningMacro(
      "GlobalIds attribute from " << cellData->GetClassName() << " should not be nullptr");
    return EXIT_FAILURE;
  }
  vtkIdType globalIdsArraySize = globalIdsArray->GetDataSize();
  if (globalIdsArraySize != nbTuples)
  {
    vtkGenericWarningMacro(<< "Wrong size for GlobalIds attribute from " << cellData->GetClassName()
                           << ". Should be: " << nbTuples << " but is: " << globalIdsArraySize);
    return EXIT_FAILURE;
  }
  for (vtkIdType i = 0; i < nbTuples; ++i)
  {
    if (globalIdsArray->GetTuple1(i) != i + offsets[rank])
    {
      vtkGenericWarningMacro(<< "Wrong id in GlobalIds attribute from " << cellData->GetClassName()
                             << ". Should be: " << i + offsets[rank]
                             << " but is: " << globalIdsArray->GetTuple1(i));
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
