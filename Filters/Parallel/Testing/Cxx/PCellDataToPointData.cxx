/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PCellDataToPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Tests PCellDataToPointData.

/*
** This test only builds if MPI is in use. It uses 2 MPI processes and checks
** that the vtkPCellDataToPointDataFilter works properly.
*/
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPCellDataToPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRTAnalyticSource.h"

#include <vtk_mpi.h>

int PCellDataToPointData(int argc, char* argv[])
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkMPIController* contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv, 1);

  int retVal = EXIT_SUCCESS;

  vtkMultiProcessController::SetGlobalController(contr);

  int me = contr->GetLocalProcessId();

  if (!contr->IsA("vtkMPIController"))
  {
    if (me == 0)
    {
      cout << "PCellDataToPointData test requires MPI" << endl;
    }
    contr->Delete();
    return EXIT_FAILURE;
  }

  int numProcs = contr->GetNumberOfProcesses();

  // Create and execute pipeline
  vtkRTAnalyticSource* wavelet = vtkRTAnalyticSource::New();
  vtkPointDataToCellData* pd2cd = vtkPointDataToCellData::New();
  vtkPCellDataToPointData* cd2pd = vtkPCellDataToPointData::New();
  vtkDataSetSurfaceFilter* toPolyData = vtkDataSetSurfaceFilter::New();
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();

  pd2cd->SetInputConnection(wavelet->GetOutputPort());
  cd2pd->SetInputConnection(pd2cd->GetOutputPort());
  cd2pd->SetPieceInvariant(1); // should be the default anyway
  toPolyData->SetInputConnection(cd2pd->GetOutputPort());

  mapper->SetInputConnection(toPolyData->GetOutputPort());
  mapper->SetScalarRange(0, numProcs);
  mapper->SetPiece(me);
  mapper->SetNumberOfPieces(numProcs);
  mapper->Update();

  vtkIdType correct = 5292;
  if (vtkDataSet::SafeDownCast(cd2pd->GetOutput())->GetNumberOfPoints() != correct)
  {
    vtkGenericWarningMacro("Wrong number of unstructured grid points on process "
      << me << ". Should be " << correct << " but is "
      << vtkDataSet::SafeDownCast(cd2pd->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }

  mapper->Delete();
  toPolyData->Delete();
  cd2pd->Delete();
  pd2cd->Delete();
  wavelet->Delete();

  contr->Finalize();
  contr->Delete();

  return retVal;
}
