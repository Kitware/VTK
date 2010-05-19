/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitUnstructuredGridPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransmitUnstructuredGridPiece.h"

#include "vtkCellData.h"
#include "vtkExtractUnstructuredGridPiece.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkTransmitUnstructuredGridPiece);

vtkCxxSetObjectMacro(vtkTransmitUnstructuredGridPiece,Controller,
                     vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkTransmitUnstructuredGridPiece::vtkTransmitUnstructuredGridPiece()
{
  this->CreateGhostCells = 1;

  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());  
}

//----------------------------------------------------------------------------
vtkTransmitUnstructuredGridPiece::~vtkTransmitUnstructuredGridPiece()
{
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
int vtkTransmitUnstructuredGridPiece::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info object
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  if (this->Controller == NULL)
    {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                1);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                0);
    return 1;
    }
  
  if (this->Controller->GetLocalProcessId() == 0)
    { // Request everything.
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                1);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                0);
    }
  else
    { // Request nothing.
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                0);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkTransmitUnstructuredGridPiece::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR(),
               inInfo->Get(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR()));
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);

  return 1;
}
  
//----------------------------------------------------------------------------
int vtkTransmitUnstructuredGridPiece::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkUnstructuredGrid *input = vtkUnstructuredGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int procId;

  if (this->Controller == NULL)
    {
    vtkErrorMacro("Could not find Controller.");
    return 1;
    }

  procId = this->Controller->GetLocalProcessId();
  if (procId == 0)
    {
    // cerr << "Root Execute\n";
    this->RootExecute(input, output, outInfo);
    }
  else
    {
    // cerr << "Satellite Execute " << procId << endl;
    this->SatelliteExecute(procId, output, outInfo);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkTransmitUnstructuredGridPiece::RootExecute(vtkUnstructuredGrid *input,
                                                   vtkUnstructuredGrid *output,
                                                   vtkInformation *outInfo)
{
  vtkUnstructuredGrid *tmp = vtkUnstructuredGrid::New();
  vtkExtractUnstructuredGridPiece *extract = 
    vtkExtractUnstructuredGridPiece::New();
  int ext[3];
  int numProcs, i;

  int outPiece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  if (outPiece != 0)
    {
    vtkWarningMacro(<< "Piece " << outPiece
                    << " does not match process 0.  " 
                    << "Altering request to try to avoid a deadlock.");
    }

  vtkStreamingDemandDrivenPipeline *extractExecutive =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(extract->GetExecutive());

  // First, set up the pipeline and handle local request.
  tmp->ShallowCopy(input);
  tmp->SetReleaseDataFlag(0);
  extract->SetCreateGhostCells(this->CreateGhostCells);
  extract->SetInput(tmp);
  extractExecutive->UpdateDataObject();

  vtkInformation *extractOutInfo = extractExecutive->GetOutputInformation(0);
  extractOutInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  extractOutInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  extractOutInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  extractOutInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), 1);

  extract->Update();

  // Copy geometry without copying information.
  output->CopyStructure(extract->GetOutput());
  output->GetPointData()->PassData(extract->GetOutput()->GetPointData());
  output->GetCellData()->PassData(extract->GetOutput()->GetCellData());
  vtkFieldData*  inFd = extract->GetOutput()->GetFieldData();
  vtkFieldData* outFd = output->GetFieldData();
  if (inFd && outFd)
    {
    outFd->PassData(inFd);
    }

  // Now do each of the satellite requests.
  numProcs = this->Controller->GetNumberOfProcesses();
  for (i = 1; i < numProcs; ++i)
    {
    this->Controller->Receive(ext, 3, i, 22341);
    extractOutInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                        ext[1]);
    extractOutInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                        ext[0]);
    extractOutInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                        ext[2]);
    extract->Update();
    this->Controller->Send(extract->GetOutput(), i, 22342);
    }
  tmp->Delete();
  extract->Delete();
}

//----------------------------------------------------------------------------
void vtkTransmitUnstructuredGridPiece::SatelliteExecute(
  int, vtkUnstructuredGrid *output, vtkInformation *outInfo)
{
  vtkUnstructuredGrid *tmp = vtkUnstructuredGrid::New();
  int ext[3];

  ext[0] = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  ext[1] = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ext[2] = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  this->Controller->Send(ext, 3, 0, 22341);
  this->Controller->Receive(tmp, 0, 22342);

  // Copy geometry without copying information.
  output->CopyStructure(tmp);
  output->GetPointData()->PassData(tmp->GetPointData());
  output->GetCellData()->PassData(tmp->GetCellData());

  tmp->Delete();
}

//----------------------------------------------------------------------------
void vtkTransmitUnstructuredGridPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");
  
  os << indent << "Controller: (" << this->Controller << ")\n";

}

