// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Tests vtkGenerateProcessIds and the new ProcessIds dataset attribute.

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkGenerateProcessIds.h"
#include "vtkIdTypeArray.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"

static int TestGenerator(vtkDataSetAttributes* dataSetAttributes, int rank);

int TestGenerateProcessIds(int argc, char* argv[])
{
  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller);

  // Create and execute pipeline
  vtkNew<vtkRTAnalyticSource> wavelet;
  vtkNew<vtkGenerateProcessIds> pidGenerator;
  pidGenerator->SetInputConnection(wavelet->GetOutputPort());
  pidGenerator->GenerateCellDataOn();
  pidGenerator->Update();

  vtkDataSet* pidOutput = pidGenerator->GetOutput();
  int myRank = controller->GetLocalProcessId();

  int retVal = TestGenerator(pidOutput->GetPointData(), myRank);
  retVal =
    (TestGenerator(pidOutput->GetCellData(), myRank) == EXIT_FAILURE) ? EXIT_FAILURE : retVal;

  controller->Finalize();
  return retVal;
}

int TestGenerator(vtkDataSetAttributes* dataSetAttributes, int rank)
{
  vtkDataArray* pidDataArray = dataSetAttributes->GetProcessIds();
  vtkIdType nbTuples = dataSetAttributes->GetNumberOfTuples();

  if (!pidDataArray)
  {
    vtkGenericWarningMacro("ProcessIds attribute from " << dataSetAttributes->GetClassName()
                                                        << " should not be nullptr");
    return EXIT_FAILURE;
  }
  vtkIdTypeArray* pidArray = vtkIdTypeArray::SafeDownCast(pidDataArray);
  if (pidArray)
  {
    vtkIdType pidArraySize = pidArray->GetDataSize();
    if (pidArraySize != nbTuples)
    {
      vtkGenericWarningMacro(<< "Wrong size for ProcessIds attribute from "
                             << dataSetAttributes->GetClassName() << ". Should be: " << nbTuples
                             << " but is: " << pidArraySize);
      return EXIT_FAILURE;
    }
    for (vtkIdType i = 0; i < nbTuples; ++i)
    {
      if (pidArray->GetValue(i) != rank)
      {
        vtkGenericWarningMacro(<< "Wrong id in ProcessIds attribute from "
                               << dataSetAttributes->GetClassName() << ". Should be: " << rank
                               << " but is: " << pidArray->GetValue(i));
        return EXIT_FAILURE;
      }
    }
  }
  else
  {
    vtkGenericWarningMacro(<< "ProcessIds attribute from " << dataSetAttributes->GetClassName()
                           << " should be of type: vtkIdTypeArray, "
                           << "but is of type: " << pidDataArray->GetClassName());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
