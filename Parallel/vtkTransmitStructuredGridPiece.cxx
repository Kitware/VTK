/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitStructuredGridPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransmitStructuredGridPiece.h"

#include "vtkCellData.h"
#include "vtkExtractGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkDoubleArray.h"

vtkStandardNewMacro(vtkTransmitStructuredGridPiece);

vtkCxxSetObjectMacro(vtkTransmitStructuredGridPiece,Controller,
                     vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkTransmitStructuredGridPiece::vtkTransmitStructuredGridPiece()
{
  this->Controller = NULL;
  this->CreateGhostCells = 1;
  this->SetNumberOfInputPorts(1);
  this->SetController(vtkMultiProcessController::GetGlobalController());  
  if (this->Controller) 
    {
    if (this->Controller->GetLocalProcessId() != 0) 
      {
      this->SetNumberOfInputPorts(0);
      }
    }
}

//----------------------------------------------------------------------------
vtkTransmitStructuredGridPiece::~vtkTransmitStructuredGridPiece()
{
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
int vtkTransmitStructuredGridPiece::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (this->Controller == NULL) 
    {
    return 1;
    }
  else 
    {
    int wExtent[6] = {0,-1,0,-1,0,-1};
    int dims[3];

    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    if (this->Controller->GetLocalProcessId() == 0)
      {
      //Root sends meta-information to the satellites.

      vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExtent);

      vtkStructuredGrid *input = vtkStructuredGrid::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
      input->GetDimensions(dims);

      int numProcs = this->Controller->GetNumberOfProcesses();
      for (int i = 1; i < numProcs; ++i)
        {
        this->Controller->Send(wExtent, 6, i, 22342);
        this->Controller->Send(dims, 3, i, 22342);
        }
      }
    else
      {
      //Satellites ask root for meta-info, because they do not read it themselves.
      
      this->Controller->Receive(wExtent, 6, 0, 22342);
      this->Controller->Receive(dims, 3, 0, 22342);

      vtkStructuredGrid *output = vtkStructuredGrid::SafeDownCast(
         outInfo->Get(vtkDataObject::DATA_OBJECT()));

      output->SetExtent(wExtent);
      output->SetDimensions(dims);
      }

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                 wExtent,
                 6);

    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkTransmitStructuredGridPiece::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  if (this->Controller == NULL)
    {
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 
                6);

    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                0);
    return 1;
    }
  
  if (this->Controller->GetLocalProcessId() == 0)
    { // Request everything.
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 
                6);

    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                0);
    }
  else
    { // Request nothing from input, will get it from root
    }

  return 1;
}

  
//----------------------------------------------------------------------------
int vtkTransmitStructuredGridPiece::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkStructuredGrid *output = vtkStructuredGrid::SafeDownCast(
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
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    vtkStructuredGrid *input = vtkStructuredGrid::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));

    this->RootExecute(input, output, outInfo);
    }
  else
    {
    this->SatelliteExecute(procId, output, outInfo);
    }

  int ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  
  if (ghostLevel > 0 && this->CreateGhostCells)
    {
    output->GenerateGhostLevelArray();
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkTransmitStructuredGridPiece::RootExecute(vtkStructuredGrid *input,
                                                   vtkStructuredGrid *output,
                                                   vtkInformation *outInfo)
{
  vtkStructuredGrid *tmp = vtkStructuredGrid::New();
  vtkExtractGrid *extract = vtkExtractGrid::New();
  int ext[7];
  int numProcs, i;

  int outExtent[6];  
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExtent);

  vtkStreamingDemandDrivenPipeline *extractExecutive =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(extract->GetExecutive());

  // First, set up the pipeline and handle local request.
  tmp->ShallowCopy(input);
  tmp->SetReleaseDataFlag(0);
  extract->SetInput(tmp);
  extractExecutive->UpdateDataObject();

  vtkInformation *extractOutInfo = extractExecutive->GetOutputInformation(0);

  extractOutInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
    6);
  extractOutInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  extractOutInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), 1);
  extract->Update();

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
    this->Controller->Receive(ext, 7, i, 22341);
    extractOutInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                        ext,
                        6);
    extractOutInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                        ext[6]);
    extract->Modified();
    extract->Update();
    this->Controller->Send(extract->GetOutput(), i, 22342);
    }

  //clean up the structures we've used here
  tmp->Delete();
  extract->Delete();
}

//----------------------------------------------------------------------------
void vtkTransmitStructuredGridPiece::SatelliteExecute(
  int, vtkStructuredGrid *output, vtkInformation *outInfo)
{
  vtkStructuredGrid *tmp = vtkStructuredGrid::New();

  //decide what we want to ask for and ask root for it
  int uExtent[7];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExtent);
  uExtent[6] = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  this->Controller->Send(uExtent, 7, 0, 22341);

  int wExtent[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExtent);

  //recieve root's response
  this->Controller->Receive(tmp, 0, 22342);

  // Retrieve Structure within requested region.
  int ext[6];
  tmp->GetExtent(ext);  
  output->SetExtent(wExtent);

  int wsizek = wExtent[5]-wExtent[4]+1;
  int wsizej = wExtent[3]-wExtent[2]+1;
  int wsizei = wExtent[1]-wExtent[0]+1;
  int wsize  = wsizek*wsizej*wsizei; 
  int wcsize  = (wsizek-1)*(wsizej-1)*(wsizei-1); 

  vtkPoints *ip = tmp->GetPoints();  
  vtkPoints *op = vtkPoints::New();
  op->SetNumberOfPoints(wsize);

  double coords[3];
  vtkIdType pCtr = 0;
  for (int k = uExtent[4]; k <= uExtent[5]; k++) 
    {
    for (int j = uExtent[2]; j <= uExtent[3]; j++) 
      {
      for (int i = uExtent[0]; i <= uExtent[1]; i++) 
        {
        vtkIdType pointId = k*wsizej*wsizei +  j*wsizei + i;
        ip->GetPoint(pCtr++, coords);
        op->SetPoint(pointId, coords);
        }
      }        
    }
  op->Squeeze();
  output->SetPoints(op);
  op->Delete();
  
  // Retrieve attributes within requested region.
  vtkPointData *ipd = tmp->GetPointData();  
  vtkPointData *opd = output->GetPointData();
  opd->CopyAllocate(ipd, wsize, 1000);

  vtkCellData *icd = tmp->GetCellData();
  vtkCellData *ocd = output->GetCellData();
  ocd->CopyAllocate(icd, wcsize, 1000);

  vtkIdType ptCtr = 0;
  vtkIdType clCtr = 0;
  for (int k = uExtent[4]; k <= uExtent[5]; k++) 
    {
    for (int j = uExtent[2]; j <= uExtent[3]; j++) 
      {
      for (int i = uExtent[0]; i <= uExtent[1]; i++) 
        {
        vtkIdType pointId = k*wsizej*wsizei +  j*wsizei + i;
        opd->CopyData(ipd, ptCtr++, pointId);
        if ((k != uExtent[5]) && (j != uExtent[3]) && (i != uExtent[1]))
          {
          vtkIdType cellId = k*(wsizej-1)*(wsizei-1) +  j*(wsizei-1) + i;
          ocd->CopyData(icd, clCtr++, cellId);
          }
        }
      }        
    }

  // Retrieve field data.
  vtkFieldData*  inFd = tmp->GetFieldData();
  vtkFieldData* outFd = output->GetFieldData();
  if (inFd && outFd)
    {
    outFd->PassData(inFd);
    }

  //clean up the structures we've used here
  tmp->Delete();
}

//----------------------------------------------------------------------------
void vtkTransmitStructuredGridPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");
  
  os << indent << "Controller: (" << this->Controller << ")\n";

}

