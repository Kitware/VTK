/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPExtractGrid.h"

// VTK includes
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIController.h"
#include "vtkMPIUtilities.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredImplicitConnectivity.h"

// C/C++ includes
#include <cassert>
#include <sstream>

// #define DEBUG

vtkStandardNewMacro(vtkPExtractGrid);

//------------------------------------------------------------------------------
vtkPExtractGrid::vtkPExtractGrid()
{
  this->Controller = vtkMPIController::SafeDownCast(
      vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPExtractGrid::~vtkPExtractGrid()
{

}

//------------------------------------------------------------------------------
void vtkPExtractGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//------------------------------------------------------------------------------
int vtkPExtractGrid::RequestData(
        vtkInformation* request,
        vtkInformationVector** inputVector,
        vtkInformationVector* outputVector)
{
  // STEP 0: Execute superclass
  int rc = this->Superclass::RequestData(request,inputVector,outputVector);
  if( rc < 0 )
    {
    return( rc );
    }

  // STEP 1: Get output information
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  assert("pre: output invformation is NULL!" && (outInfo != NULL) );

  // STEP 2: Get the whole extent
  int wholeExtent[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExtent);

#ifdef DEBUG
  vtkMPIUtilities::SynchronizedPrintf(
      this->Controller,"wholeExtent=[%d,%d,%d,%d,%d,%d]\n",
      wholeExtent[0],wholeExtent[1],wholeExtent[2],
      wholeExtent[3],wholeExtent[4],wholeExtent[5]);
#endif

  // STEP 3: Get the structured grid instance in this process
  vtkStructuredGrid* grid = vtkStructuredGrid::SafeDownCast(
          outInfo->Get(vtkDataObject::DATA_OBJECT()));

#ifdef DEBUG
  int myExt[6];
  grid->GetExtent(myExt);

  vtkMPIUtilities::SynchronizedPrintf(
      this->Controller,"ext=[%d,%d,%d,%d,%d,%d]\n",
      myExt[0],myExt[1],myExt[2],myExt[3],myExt[4],myExt[5]);
#endif

  // STEP 4: Detect & resolve gaps if any
  vtkStructuredImplicitConnectivity* gridConnectivity =
      vtkStructuredImplicitConnectivity::New();
  gridConnectivity->SetWholeExtent(wholeExtent);

  // Register the grid, grid ID is the same as the process ID
  gridConnectivity->RegisterGrid(
    this->Controller->GetLocalProcessId(),
    grid->GetExtent(),
    grid->GetPoints(),
    grid->GetPointData()
    );

  // Establish neighbor connectivity & detect any gaps
  gridConnectivity->EstablishConnectivity();

  // Check if there are any gaps, if any close them now
  if( gridConnectivity->HasImplicitConnectivity() )
    {
#ifdef DEBUG
    vtkMPIUtilities::Printf(this->Controller, "Closing Gap...\n");
#endif
    // there are gaps, grow the grid to the right
    gridConnectivity->ExchangeData();
    gridConnectivity->GetOutputStructuredGrid(
      this->Controller->GetLocalProcessId(),grid);
    }

#ifdef DEBUG
  grid->GetExtent(myExt);
  vtkMPIUtilities::SynchronizedPrintf(
      this->Controller,"ext=[%d,%d,%d,%d,%d,%d]\n",
      myExt[0],myExt[1],myExt[2],myExt[3],myExt[4],myExt[5]);
#endif

  gridConnectivity->Delete();
  return( rc );
}

//------------------------------------------------------------------------------
int vtkPExtractGrid::RequestInformation(
        vtkInformation* request,
        vtkInformationVector** inputVector,
        vtkInformationVector* outputVector)
{
  if (!this->Controller)
    {
    vtkErrorMacro("This filter needs a controller to work.");
    return 0;
    }
  int rc = this->Superclass::RequestInformation(
              request,inputVector,outputVector);
  return( rc );
}

//------------------------------------------------------------------------------
int vtkPExtractGrid::RequestUpdateExtent(
        vtkInformation* request,
        vtkInformationVector** inputVector,
        vtkInformationVector* outputVector)
{
  int rc = this->Superclass::RequestUpdateExtent(
              request,inputVector,outputVector);
  return( rc );
}
