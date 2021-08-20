/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistancePolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkDummyController.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataGroupFilter.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPOutlineFilter.h"
#include "vtkRTAnalyticSource.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include <iostream>
int TestPOutlineFilter(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkMPIController* contr = vtkMPIController::New();
#else
  vtkDummyController* contr = vtkDummyController::New();
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  int me = contr->GetLocalProcessId();
  int numProcs = contr->GetNumberOfProcesses();
  int localExtent[6] = { (me - 1) * 10, me * 10, -10, 10, -10, 10 };

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->UpdatePiece(me, numProcs, 0, localExtent);

  vtkNew<vtkPOutlineFilter> outlineDS;
  outlineDS->SetController(vtkMultiProcessController::GetGlobalController());
  outlineDS->SetInputConnection(wavelet->GetOutputPort());
  outlineDS->Update();

  vtkNew<vtkMultiBlockDataGroupFilter> grouper;
  grouper->AddInputConnection(wavelet->GetOutputPort());

  vtkNew<vtkPOutlineFilter> outlineMB;
  outlineMB->SetController(vtkMultiProcessController::GetGlobalController());
  outlineMB->SetInputConnection(grouper->GetOutputPort());
  outlineMB->Update();

  int retValue = EXIT_SUCCESS;
  if (me == 0)
  {
    int expectedNumberOfPoints = 8;
    int expectedNumberOfCells = 12;
    if (outlineDS->GetOutput()->GetNumberOfPoints() != expectedNumberOfPoints ||
      outlineMB->GetOutput()->GetNumberOfPoints() != expectedNumberOfPoints ||
      outlineDS->GetOutput()->GetNumberOfCells() != expectedNumberOfCells ||
      outlineMB->GetOutput()->GetNumberOfCells() != expectedNumberOfCells)
    {
      std::cerr << "ERROR: Unexpected number of points or cells" << std::endl;
      retValue = EXIT_FAILURE;
    }
  }

  contr->Finalize();
  contr->Delete();
  return retValue;
}
