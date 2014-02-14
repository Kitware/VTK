/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitPolyDataPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransmitPolyDataPiece.h"

#include "vtkCellData.h"
#include "vtkExtractPolyDataPiece.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkTransmitPolyDataPiece);

vtkCxxSetObjectMacro(vtkTransmitPolyDataPiece,Controller,
                     vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkTransmitPolyDataPiece::vtkTransmitPolyDataPiece()
{
  this->CreateGhostCells = 1;

  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkTransmitPolyDataPiece::~vtkTransmitPolyDataPiece()
{
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
int vtkTransmitPolyDataPiece::RequestData(
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

  int procId;
  if (this->Controller == NULL)
    {
    vtkErrorMacro("Could not find Controller.");
    return 0;
    }

  procId = this->Controller->GetLocalProcessId();
  if (procId == 0)
    {
    // It is important to synchronize these calls (all processes execute)
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
void vtkTransmitPolyDataPiece::RootExecute(vtkPolyData *input,
                                           vtkPolyData *output,
                                           vtkInformation *outInfo)
{
  vtkPolyData *tmp = vtkPolyData::New();
  vtkExtractPolyDataPiece *extract = vtkExtractPolyDataPiece::New();
  int ext[3];
  int numProcs, i;

  vtkStreamingDemandDrivenPipeline *extractExecutive =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(extract->GetExecutive());
  vtkInformation *extractInfo = extractExecutive->GetOutputInformation(0);

  // First, set up the pipeline and handle local request.
  tmp->ShallowCopy(input);
  extract->SetCreateGhostCells(this->CreateGhostCells);
  extract->SetInputData(tmp);

  extractExecutive->UpdateDataObject();
  extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                   outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                   outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                   outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  extractInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), 1);
  extract->Update();

  // Copy geometry without copying information.
  output->CopyStructure(extract->GetOutput());
  output->GetPointData()->PassData(extract->GetOutput()->GetPointData());
  output->GetCellData()->PassData(extract->GetOutput()->GetCellData());
  output->GetFieldData()->PassData(extract->GetOutput()->GetFieldData());

  // Now do each of the satellite requests.
  numProcs = this->Controller->GetNumberOfProcesses();
  for (i = 1; i < numProcs; ++i)
    {
    this->Controller->Receive(ext, 3, i, 22341);
    extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                     ext[1]);
    extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                     ext[0]);
    extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                     ext[2]);
    extract->Update();
    this->Controller->Send(extract->GetOutput(), i, 22342);
    }
  tmp->Delete();
  extract->Delete();
}

//----------------------------------------------------------------------------
void vtkTransmitPolyDataPiece::SatelliteExecute(int, vtkPolyData *output,
                                                vtkInformation *outInfo)
{
  vtkPolyData *tmp = vtkPolyData::New();
  int ext[3];

  ext[0] =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  ext[1] =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ext[2] =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  this->Controller->Send(ext, 3, 0, 22341);
  this->Controller->Receive(tmp, 0, 22342);

  // Copy geometry without copying information.
  output->CopyStructure(tmp);
  output->GetPointData()->PassData(tmp->GetPointData());
  output->GetCellData()->PassData(tmp->GetCellData());
  output->GetFieldData()->PassData(tmp->GetFieldData());

  tmp->Delete();
}

//----------------------------------------------------------------------------
void vtkTransmitPolyDataPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");

  os << indent << "Controller: (" << this->Controller << ")\n";

}
