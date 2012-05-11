/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollectTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkCollectTable.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkSocketController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVariant.h"

vtkStandardNewMacro(vtkCollectTable);

vtkCxxSetObjectMacro(vtkCollectTable,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkCollectTable,SocketController, vtkSocketController);

//----------------------------------------------------------------------------
vtkCollectTable::vtkCollectTable()
{
  this->PassThrough = 0;
  this->SocketController = NULL;

  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkCollectTable::~vtkCollectTable()
{
  this->SetController(0);
  this->SetSocketController(0);
}

//----------------------------------------------------------------------------
int vtkCollectTable::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);

  return 1;
}

//--------------------------------------------------------------------------
int vtkCollectTable::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
              outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
              outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
              outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  return 1;
}

//----------------------------------------------------------------------------
int vtkCollectTable::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkTable *input = vtkTable::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTable *output = vtkTable::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int numProcs, myId;
  int idx;

  if (this->Controller == NULL && this->SocketController == NULL)
    { // Running as a single process.
    output->ShallowCopy(input);
    return 1;
    }

  if (this->Controller == NULL && this->SocketController != NULL)
    { // This is a client.  We assume no data on client for input.
    if ( ! this->PassThrough)
      {
      vtkTable* table = NULL;;
      table = vtkTable::New();
      this->SocketController->Receive(table, 1, 121767);
      output->ShallowCopy(table);
      table->Delete();
      table = NULL;
      return 1;
      }
    // If not collected, output will be empty from initialization.
    return 0;
    }

  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();

  if (this->PassThrough)
    {
    // Just copy and return (no collection).
    output->ShallowCopy(input);
    return 1;
    }

  // Collect.
  if (myId == 0)
    {
    vtkTable* wholeTable = vtkTable::New();
    wholeTable->ShallowCopy(input);

    for (idx = 1; idx < numProcs; ++idx)
      {
      vtkTable* curTable = vtkTable::New();
      this->Controller->Receive(curTable, idx, 121767);
      vtkIdType numRows = curTable->GetNumberOfRows();
      vtkIdType numCols = curTable->GetNumberOfColumns();
      for (vtkIdType i = 0; i < numRows; i++)
        {
        vtkIdType curRow = wholeTable->InsertNextBlankRow();
        for (vtkIdType j = 0; j < numCols; j++)
          {
          wholeTable->SetValue(curRow, j, curTable->GetValue(i, j));
          }
        }
      curTable->Delete();
      }

    if (this->SocketController)
      { // Send collected data onto client.
      this->SocketController->Send(wholeTable, 1, 121767);
      // output will be empty.
      }
    else
      { // No client. Keep the output here.
      output->ShallowCopy(wholeTable);
      }
    }
  else
    {
    this->Controller->Send(input, 0, 121767);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkCollectTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PassThough: " << this->PassThrough << endl;
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "SocketController: (" << this->SocketController << ")\n";
}
