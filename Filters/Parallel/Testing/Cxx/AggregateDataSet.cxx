/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AggregateDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Tests vtkAggregateDataSetFilter.

/*
** This test only builds if MPI is in use. It uses 4 MPI processes to
** test that the data is aggregated down to two processes. It also uses
** rendering to generate the pieces on each process but uses a simple
** point count to verify results. The VTK XML writers could have been
** used but this module is not dependent on those writers yet so adding
** yet another dependency seemed bad.
*/
#include "vtkAggregateDataSetFilter.h"
#include "vtkContourFilter.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkThresholdPoints.h"

#include <mpi.h>

int AggregateDataSet(int argc, char *argv[])
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkMPIController *contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv, 1);

  int retVal = EXIT_SUCCESS;

  vtkMultiProcessController::SetGlobalController(contr);

  int me = contr->GetLocalProcessId();

  if (!contr->IsA("vtkMPIController"))
  {
    if (me == 0)
    {
      cout << "AggregateDataSet test requires MPI" << endl;
    }
    contr->Delete();
    return EXIT_FAILURE;
  }

  int numProcs=contr->GetNumberOfProcesses();

  // Create and execute pipeline
  vtkRTAnalyticSource* wavelet = vtkRTAnalyticSource::New();
  vtkDataSetSurfaceFilter* toPolyData = vtkDataSetSurfaceFilter::New();
  vtkAggregateDataSetFilter* aggregate = vtkAggregateDataSetFilter::New();
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();

  toPolyData->SetInputConnection(wavelet->GetOutputPort());
  aggregate->SetInputConnection(toPolyData->GetOutputPort());
  aggregate->SetNumberOfTargetProcesses(2);

  mapper->SetInputConnection(aggregate->GetOutputPort());
  mapper->SetScalarRange(0, numProcs);
  mapper->SetPiece(me);
  mapper->SetNumberOfPieces(numProcs);
  mapper->Update();

  if (me % 2 == 0 && vtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints() != 1408)
  {
    vtkGenericWarningMacro("Wrong number of polydata points on process " << me
                           << ". Should be 1408 but is " <<
                           vtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }
  else if (me % 2 != 0 && vtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints() != 0)
  {
    vtkGenericWarningMacro("Wrong number of polydata points on process " << me << ". Should be 0 but is " <<
                           vtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }

  vtkThresholdPoints* threshold = vtkThresholdPoints::New();;
  threshold->ThresholdBetween(0, 500);
  threshold->SetInputConnection(wavelet->GetOutputPort());
  aggregate->SetInputConnection(threshold->GetOutputPort());

  vtkContourFilter* contour = vtkContourFilter::New();
  double scalar_range[2] = {50, 400};
  contour->GenerateValues(5, scalar_range);
  contour->SetInputConnection(aggregate->GetOutputPort());
  mapper->SetInputConnection(contour->GetOutputPort());
  mapper->Update();

  if (me % 2 == 0 && vtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints() != 5082)
  {
    vtkGenericWarningMacro("Wrong number of unstructured grid points on process "
                           << me << ". Should be 5082 but is " <<
                           vtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }
  else if (me % 2 != 0 && vtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints() != 0)
  {
    vtkGenericWarningMacro("Wrong number of unstructured grid points on process " << me << ". Should be 0 but is " <<
                           vtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }

  mapper->Delete();
  contour->Delete();
  threshold->Delete();
  aggregate->Delete();
  toPolyData->Delete();
  wavelet->Delete();

  contr->Finalize();
  contr->Delete();

  return retVal;
}
