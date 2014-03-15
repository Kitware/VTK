/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPStructuredGridConnectivity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkExtentRCBPartitioner.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkMPIController.h"
#include "vtkMPIUtilities.h"
#include "vtkMultiProcessController.h"
#include "vtkPUnstructuredGridConnectivity.h"
#include "vtkPUnstructuredGridGhostDataGenerator.h"
#include "vtkPointData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStructuredData.h"
#include "vtkTimerLog.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridWriter.h"

#include <sstream>
#include <string>

//#define DEBUG
#include "UnstructuredGhostZonesCommon.h"

//------------------------------------------------------------------------------
// Program main
int TestPUnstructuredGridGhostDataGenerator(int argc, char* argv[])
{
  int rc             = 0;
  double ellapsed    = 0.0;
  vtkTimerLog* timer = vtkTimerLog::New();

  // STEP 0: Initialize
  vtkMPIController* cntrl = vtkMPIController::New();
  cntrl->Initialize( &argc, &argv, 0 );
  vtkMultiProcessController::SetGlobalController( cntrl );
  global::Rank   = cntrl->GetLocalProcessId();
  global::NRanks = cntrl->GetNumberOfProcesses();

  // STEP 1: Generate grid in parallel in each process
  global::Grid = vtkUnstructuredGrid::New();
  GenerateDataSet();

  // STEP 2: Setup ghost data generator
  vtkPUnstructuredGridGhostDataGenerator* ghostGenerator =
      vtkPUnstructuredGridGhostDataGenerator::New();
  ghostGenerator->SetInputData(global::Grid);

  // STEP 3: Update ghost zones
  std::ostringstream grdfname;   // input grid name at each iteration for I/O
  std::ostringstream ghostfname; // ghosted grid name at each iteration for I/O
  for(int i=0; i < 2; ++i)
    {
    grdfname.clear();
    grdfname.str("");
    grdfname << "INITIAL-T" << i;

    ghostfname.clear();
    ghostfname.str("");
    ghostfname << "GHOSTED-T" << i;

    // update grid in this iteration...
    UpdateGrid(i);
    global::Grid->Modified();
#ifdef DEBUG
    WriteDataSet(global::Grid,grdfname.str().c_str());
#endif

    // update ghost zones in this iteration...
    vtkMPIUtilities::Printf(cntrl,"[INFO]: iteration=%d\n",i);
    vtkMPIUtilities::Printf(cntrl,"[INFO]: Update ghost zones...");
    timer->StartTimer();

    ghostGenerator->Update();

    timer->StopTimer();
    ellapsed = timer->GetElapsedTime();
    vtkMPIUtilities::Printf(cntrl,"[DONE]\n");

    // get some performance statistics
    double minGhostUpdateTime = 0.0;
    double maxGhostUpdateTime = 0.0;
    double avgGhostUpdateTime = 0.0;
    cntrl->Reduce(&ellapsed,&minGhostUpdateTime,1,vtkCommunicator::MIN_OP,0);
    cntrl->Reduce(&ellapsed,&maxGhostUpdateTime,1,vtkCommunicator::MAX_OP,0);
    cntrl->Reduce(&ellapsed,&avgGhostUpdateTime,1,vtkCommunicator::SUM_OP,0);
    avgGhostUpdateTime /= static_cast<double>(cntrl->GetNumberOfProcesses());
    vtkMPIUtilities::Printf(
          cntrl,"-- Ellapsed Time: min=%f, avg=%f, max=%f\n",
          minGhostUpdateTime,avgGhostUpdateTime,maxGhostUpdateTime);

    vtkUnstructuredGrid* ghostGrid = vtkUnstructuredGrid::New();
    ghostGrid->DeepCopy(ghostGenerator->GetOutput());
#ifdef DEBUG
    assert("pre: ghost gird should not be NULL!" && (ghostGrid != NULL) );
    WriteDataSet(ghostGrid,ghostfname.str().c_str());
#endif

    rc += CheckGrid(ghostGrid,i);
    ghostGrid->Delete();
    } // END for

  // STEP 5: Delete the ghost generator
  timer->Delete();
  ghostGenerator->Delete();
  global::Grid->Delete();
  cntrl->Finalize();
  cntrl->Delete();
  return( rc );
}
