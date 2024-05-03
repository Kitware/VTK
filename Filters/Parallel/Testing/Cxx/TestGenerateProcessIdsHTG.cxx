// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Tests vtkGenerateProcessIds and the new ProcessIds dataset attribute.

#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGenerateProcessIds.h"
#include "vtkIdTypeArray.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkRandomHyperTreeGridSource.h"

static int TestGenerator(vtkCellData* dataSetAttributes, int rank);

int TestGenerateProcessIdsHTG(int argc, char* argv[])
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

  vtkNew<vtkHyperTreeGridGenerateProcessIds> pidGenerator;
  pidGenerator->SetInputConnection(htgSource->GetOutputPort());
  pidGenerator->Update();

  vtkHyperTreeGrid* pidOutput = vtkHyperTreeGrid::SafeDownCast(pidGenerator->GetOutput());
  int myRank = controller->GetLocalProcessId();

  int retVal = TestGenerator(pidOutput->GetCellData(), myRank);

  controller->Finalize();
  return retVal;
}

int TestGenerator(vtkCellData* dataSetAttributes, int rank)
{
  vtkDataArray* pidDataArray = dataSetAttributes->GetProcessIds();
  vtkIdType nbTuples = dataSetAttributes->GetNumberOfTuples();

  if (!pidDataArray)
  {
    vtkGenericWarningMacro("ProcessIds attribute from " << dataSetAttributes->GetClassName()
                                                        << " should not be nullptr");
    return EXIT_FAILURE;
  }
  vtkIdType pidArraySize = pidDataArray->GetDataSize();
  if (pidArraySize != nbTuples)
  {
    vtkGenericWarningMacro(<< "Wrong size for ProcessIds attribute from "
                           << dataSetAttributes->GetClassName() << ". Should be: " << nbTuples
                           << " but is: " << pidArraySize);
    return EXIT_FAILURE;
  }
  for (vtkIdType i = 0; i < nbTuples; ++i)
  {
    if (pidDataArray->GetTuple1(i) != rank)
    {
      vtkGenericWarningMacro(<< "Wrong id in ProcessIds attribute from "
                             << dataSetAttributes->GetClassName() << ". Should be: " << rank
                             << " but is: " << pidDataArray->GetTuple1(i));
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
