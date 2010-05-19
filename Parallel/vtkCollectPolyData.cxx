/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollectPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCollectPolyData.h"

#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSocketController.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkCollectPolyData);

vtkCxxSetObjectMacro(vtkCollectPolyData,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkCollectPolyData,SocketController, vtkSocketController);

//----------------------------------------------------------------------------
vtkCollectPolyData::vtkCollectPolyData()
{
  this->PassThrough = 0;
  this->SocketController = NULL;

  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());  
}

//----------------------------------------------------------------------------
vtkCollectPolyData::~vtkCollectPolyData()
{
  this->SetController(0);
  this->SetSocketController(0);
}

//----------------------------------------------------------------------------
int vtkCollectPolyData::RequestInformation(
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
int vtkCollectPolyData::RequestUpdateExtent(
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
int vtkCollectPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int numProcs, myId;
  int idx;

  if (this->Controller == NULL && this->SocketController == NULL)
    { // Running as a single process.
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return 1;
    }

  if (this->Controller == NULL && this->SocketController != NULL)
    { // This is a client.  We assume no data on client for input.
    if ( ! this->PassThrough)
      {
      vtkPolyData *pd = NULL;;
      pd = vtkPolyData::New();
      this->SocketController->Receive(pd, 1, 121767);
      output->CopyStructure(pd);
      output->GetPointData()->PassData(pd->GetPointData());
      output->GetCellData()->PassData(pd->GetCellData());
      pd->Delete();
      pd = NULL;
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
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return 1;
    }

  // Collect.
  vtkAppendPolyData *append = vtkAppendPolyData::New();
  vtkPolyData *pd = NULL;;

  if (myId == 0)
    {
    pd = vtkPolyData::New();
    pd->CopyStructure(input);
    pd->GetPointData()->PassData(input->GetPointData());
    pd->GetCellData()->PassData(input->GetCellData());
    append->AddInput(pd);
    pd->Delete();
    for (idx = 1; idx < numProcs; ++idx)
      {
      pd = vtkPolyData::New();
      this->Controller->Receive(pd, idx, 121767);
      append->AddInput(pd);
      pd->Delete();
      pd = NULL;
      }
    append->Update();
    input = append->GetOutput();
    if (this->SocketController)
      { // Send collected data onto client.
      this->SocketController->Send(input, 1, 121767);
      // output will be empty.
      }
    else
      { // No client. Keep the output here.
      output->CopyStructure(input);
      output->GetPointData()->PassData(input->GetPointData());
      output->GetCellData()->PassData(input->GetCellData());
      }
    append->Delete();
    append = NULL;
    }
  else
    {
    this->Controller->Send(input, 0, 121767);
    append->Delete();
    append = NULL;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkCollectPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "PassThough: " << this->PassThrough << endl;
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "SocketController: (" << this->SocketController << ")\n";
}
