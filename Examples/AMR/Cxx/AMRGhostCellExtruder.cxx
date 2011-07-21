/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AMRGhostCellExtruder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME AMRGhostCellExtruder.cxx -- Ghost layer extruder
//
// .SECTION Description
// This utility will read in an AMR dataset in *.vth and generate the
// corresponding extruded dataset.

#include <cmath>
#include <sstream>
#include <cassert>
#include <mpi.h>

#include "vtkHierarchicalBoxDataSet.h"
#include "vtkXMLHierarchicalBoxDataWriter.h"
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include "vtkAMRUtilities.h"
#include "vtkAMRGhostCellExtruder.h"
#include "AMRCommon.h"

// Global Variables
vtkMultiProcessController *Controller;

int main( int argc, char **argv )
{
  Controller = vtkMPIController::New();
  Controller->Initialize( &argc, &argv );
  assert("pre: Controller != NULL" && (Controller != NULL ) );

  std::cout << "Reading AMR dataset...";
  std::cout.flush();
  vtkHierarchicalBoxDataSet *amrData=
   AMRCommon::ReadAMRData( std::string(argv[1] ) );
  assert( "pre: input AMR dataset is NULL" && (amrData != NULL) );
  std::cout << "[DONE]\n";
  std::cout.flush();

  std::cout << "Number of Ghost layers: ";
  int NG = atoi( argv[2] );
  std::cout << NG << std::endl;
  std::cout.flush();

  std::cout << "Extruding...";
  std::cout.flush();
  vtkAMRGhostCellExtruder *gcExtrusion = vtkAMRGhostCellExtruder::New();
  gcExtrusion->SetInput(amrData);
  gcExtrusion->SetNumberOfGhostLayers( NG );
  gcExtrusion->Update();
  std::cout << "[DONE]\n";
  std::cout.flush();

  vtkHierarchicalBoxDataSet *extrudedAMR = gcExtrusion->GetOutput();
  assert( "post: output AMR dataset is NULL" && (extrudedAMR != NULL) );

  AMRCommon::WriteAMRData( extrudedAMR, "EXTRUDED" );


  gcExtrusion->Delete();
  extrudedAMR->Delete();
  amrData->Delete();

  Controller->Finalize();
  Controller->Delete();
  return 0;
}
