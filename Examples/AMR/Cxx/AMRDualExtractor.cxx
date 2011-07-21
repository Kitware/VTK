/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AMRDualExtractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME AMRDualExtractor.cxx -- Extracts the dual mesh from AMR data
//
// .SECTION Description
//  This utility will read in AMR data, in *.vth native paraview format,
//  and extract the corresponding mesh dual.

#include <cmath>
#include <sstream>
#include <cassert>
#include <mpi.h>

#include "vtkUniformGrid.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkAMRBox.h"
#include "vtkAMRConnectivityFilter.h"
#include "vtkAMRGhostExchange.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkXMLHierarchicalBoxDataWriter.h"
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include "vtkAMRUtilities.h"
#include "vtkAMRDualMeshExtractor.h"

#include "AMRCommon.h"

// Global Variables
vtkMultiProcessController *Controller;

int main( int argc, char **argv )
{
  Controller = vtkMPIController::New();
  Controller->Initialize( &argc, &argv );
  assert("pre: Controller != NULL" && (Controller != NULL ) );

  // STEP 0: Read in AMR dataset
  vtkHierarchicalBoxDataSet *amrData=
    AMRCommon::ReadAMRData( std::string(argv[1] ) );
  AMRCommon::WriteAMRData( amrData, std::string("INPUTAMR") );
  assert( "AMR dataset is empty!"  && (amrData->GetNumberOfLevels() > 0) );

  // STEP 1: Compute intergrid connectivity
  std::cout << "Computing inter-block & inter-process connectivity!\n";
  std::cout.flush( );
  vtkAMRConnectivityFilter* connectivityFilter=vtkAMRConnectivityFilter::New( );
  connectivityFilter->SetController( Controller );
  connectivityFilter->SetAMRDataSet( amrData );
  connectivityFilter->ComputeConnectivity();

  std::cout << "Done computing connectivity!\n";
  std::cout.flush( );
  Controller->Barrier();
  connectivityFilter->Print( std::cout );
  Controller->Barrier();

  // STEP 2: Data transfer
  std::cout << " -- Transfering solution...\n";
  std::cout.flush();


  vtkAMRGhostExchange* gridSolutionExchanger = vtkAMRGhostExchange::New();
  gridSolutionExchanger->SetAMRDataSet( amrData );
  gridSolutionExchanger->SetNumberOfGhostLayers( 1 );
  gridSolutionExchanger->SetRemoteConnectivity(
      connectivityFilter->GetRemoteConnectivity() );
  gridSolutionExchanger->SetLocalConnectivity(
      connectivityFilter->GetLocalConnectivity() );
  gridSolutionExchanger->Update();
  vtkHierarchicalBoxDataSet *newData = gridSolutionExchanger->GetOutput();
  assert( "extruded data is NULL!" && (newData != NULL) );
  AMRCommon::WriteAMRData( newData, "EXTRUDED" );

  std::cout << " -- Generating dual mesh...";
  std::cout.flush();
  vtkAMRDualMeshExtractor *dme = vtkAMRDualMeshExtractor::New();
  dme->SetInput( newData );
  dme->Update();
  std::cout << "[DONE]\n";
  std::cout.flush();

  std::cout << " -- Writing dual...";
  std::cout.flush();
  AMRCommon::WriteMultiBlockData( dme->GetOutput(), std::string("DUALMESH") );
  std::cout << "[DONE]\n";
  std::cout.flush();

  // STEP 3: CleanUp
  dme->Delete();
  amrData->Delete();
  connectivityFilter->Delete();

  Controller->Finalize();
  Controller->Delete();
  return 0;
}
