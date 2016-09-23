/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitStructuredDataPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransmitStructuredDataPiece.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataSet.h"
#include "vtkExtentTranslator.h"

vtkStandardNewMacro(vtkTransmitStructuredDataPiece);

vtkCxxSetObjectMacro(vtkTransmitStructuredDataPiece,Controller,
                     vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkTransmitStructuredDataPiece::vtkTransmitStructuredDataPiece()
{
  this->Controller = NULL;
  this->CreateGhostCells = 1;
  this->SetNumberOfInputPorts(1);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkTransmitStructuredDataPiece::~vtkTransmitStructuredDataPiece()
{
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
int vtkTransmitStructuredDataPiece::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (this->Controller)
  {
    int wExt[6];
    if (this->Controller->GetLocalProcessId() == 0)
    {
      vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExt);
    }
    this->Controller->Broadcast(wExt, 6, 0);
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExt, 6);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkTransmitStructuredDataPiece::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  if (this->Controller)
  {
    if (this->Controller->GetLocalProcessId() > 0)
    {
      int wExt[6] = {0, -1, 0, -1, 0, -1};
      inputVector[0]->GetInformationObject(0)->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), wExt, 6);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkTransmitStructuredDataPiece::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::GetData(outputVector);

  int procId;

  if (this->Controller == NULL)
  {
    vtkErrorMacro("Could not find Controller.");
    return 1;
  }

  procId = this->Controller->GetLocalProcessId();
  if (procId == 0)
  {
    vtkDataSet *input = vtkDataSet::GetData(inputVector[0]);
    this->RootExecute(input, output, outInfo);
  }
  else
  {
    this->SatelliteExecute(procId, output, outInfo);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkTransmitStructuredDataPiece::RootExecute(vtkDataSet *input,
                                                 vtkDataSet *output,
                                                 vtkInformation *outInfo)
{
  vtkDataSet *tmp = input->NewInstance();
  int numProcs, i;

  int updatePiece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int updateNumPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int updatedGhost = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  if (!this->CreateGhostCells)
  {
    updatedGhost = 0;
  }
  int* wholeExt = input->GetInformation()->Get(vtkDataObject::DATA_EXTENT());

  vtkExtentTranslator* et = vtkExtentTranslator::New();

  int newExt[6];
  et->PieceToExtentThreadSafe(updatePiece, updateNumPieces, updatedGhost,
                              wholeExt, newExt, vtkExtentTranslator::BLOCK_MODE, 0);
  output->ShallowCopy(input);
  output->Crop(newExt);

  if (updatedGhost > 0)
  {
    // Create ghost array
    int zeroExt[6];
    et->PieceToExtentThreadSafe(updatePiece, updateNumPieces, 0,
                                wholeExt, zeroExt, vtkExtentTranslator::BLOCK_MODE, 0);
    output->GenerateGhostArray(zeroExt);
  }

  numProcs = this->Controller->GetNumberOfProcesses();
  for (i = 1; i < numProcs; ++i)
  {
    int updateInfo[3];
    this->Controller->Receive(updateInfo, 3, i, 22341);
    et->PieceToExtentThreadSafe(updateInfo[0], updateInfo[1], updateInfo[2],
                                wholeExt, newExt, vtkExtentTranslator::BLOCK_MODE, 0);
    tmp->ShallowCopy(input);
    tmp->Crop(newExt);

    if (updateInfo[2] > 0)
    {
      // Create ghost array
      int zeroExt[6];
      et->PieceToExtentThreadSafe(updateInfo[0], updateInfo[1], 0,
                                  wholeExt, zeroExt, vtkExtentTranslator::BLOCK_MODE, 0);
      tmp->GenerateGhostArray(zeroExt);
    }

    this->Controller->Send(tmp, i, 22342);
  }

  //clean up the structures we've used here
  tmp->Delete();
  et->Delete();
}

//----------------------------------------------------------------------------
void vtkTransmitStructuredDataPiece::SatelliteExecute(
  int, vtkDataSet *output, vtkInformation *outInfo)
{
  int updatePiece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int updateNumPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int updatedGhost = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  if (!this->CreateGhostCells)
  {
    updatedGhost = 0;
  }

  int updateInfo[3];
  updateInfo[0] = updatePiece;
  updateInfo[1] = updateNumPieces;
  updateInfo[2] = updatedGhost;

  this->Controller->Send(updateInfo, 3, 0, 22341);

  //receive root's response
  this->Controller->Receive(output, 0, 22342);
}

//----------------------------------------------------------------------------
void vtkTransmitStructuredDataPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");

  os << indent << "Controller: (" << this->Controller << ")\n";

}
