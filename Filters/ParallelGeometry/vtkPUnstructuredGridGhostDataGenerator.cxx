/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPStructuredGridConnectivity.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPUnstructuredGridGhostDataGenerator.h"

// VTK includes
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIController.h"
#include "vtkMPIUtilities.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPUnstructuredGridConnectivity.h"
#include "vtkUnstructuredGrid.h"

// C/C++ includes
#include <cassert>

vtkStandardNewMacro(vtkPUnstructuredGridGhostDataGenerator);

//------------------------------------------------------------------------------
vtkPUnstructuredGridGhostDataGenerator::vtkPUnstructuredGridGhostDataGenerator()
{
  this->GhostZoneBuilder = NULL;
  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkPUnstructuredGridGhostDataGenerator::~vtkPUnstructuredGridGhostDataGenerator()
{
  if(this->GhostZoneBuilder != NULL)
    {
    this->GhostZoneBuilder->Delete();
    }
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridGhostDataGenerator::PrintSelf(
      ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//------------------------------------------------------------------------------
int vtkPUnstructuredGridGhostDataGenerator::FillInputPortInformation(
      int vtkNotUsed(port), vtkInformation* info)
{
  assert( "pre: information object is NULL!" && (info != NULL) );
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkUnstructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkPUnstructuredGridGhostDataGenerator::FillOutputPortInformation(
      int vtkNotUsed(port), vtkInformation* info)
{
  assert( "pre: information object is NULL!" && (info != NULL) );
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkUnstructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkPUnstructuredGridGhostDataGenerator::RequestData(
    vtkInformation* vtkNotUsed(rqst),
    vtkInformationVector** inputVector,
   vtkInformationVector* outputVector)
{
  // STEP 0: Get input grid
  vtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: input grid is NULL!" && (input != NULL) );
  vtkUnstructuredGrid* grid =
   vtkUnstructuredGrid::SafeDownCast(input->Get(vtkDataObject::DATA_OBJECT()));

  if( (grid==NULL) || (grid->GetNumberOfCells()==0) )
    {
    // empty input, do nothing
    return 1;
    }

  // STEP 1: Get output grid
  vtkInformation* output = outputVector->GetInformationObject(0);
  assert("pre: output object is NULL" && (output != NULL) );
  vtkUnstructuredGrid* ghostedGrid =
      vtkUnstructuredGrid::SafeDownCast(
          output->Get(vtkDataObject::DATA_OBJECT()));
  assert("pre: output grid object is NULL!" && (ghostedGrid != NULL) );

  // STEP 2: Build the ghost zones, if not already built
  if( this->GhostZoneBuilder == NULL )
    {
    this->GhostZoneBuilder = vtkPUnstructuredGridConnectivity::New();
    vtkMPIController* mpiController =
        vtkMPIController::SafeDownCast(this->Controller);
    assert("pre: null mpi controller!" && (mpiController != NULL) );
    this->GhostZoneBuilder->SetController(mpiController);
    this->GhostZoneBuilder->RegisterGrid( grid );
    this->GhostZoneBuilder->BuildGhostZoneConnectivity();
    }

  // STEP 3: Update the ghost zones
  this->GhostZoneBuilder->UpdateGhosts();

  // STEP 4: Get the ghosted grid
  ghostedGrid->DeepCopy(this->GhostZoneBuilder->GetGhostedGrid());
  return 1;
}
