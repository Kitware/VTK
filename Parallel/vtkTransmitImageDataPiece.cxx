/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitImageDataPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransmitImageDataPiece.h"

#include "vtkCellData.h"
#include "vtkImageClip.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkImageData.h"
#include "vtkDoubleArray.h"

vtkStandardNewMacro(vtkTransmitImageDataPiece);

vtkCxxSetObjectMacro(vtkTransmitImageDataPiece,Controller,
                     vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkTransmitImageDataPiece::vtkTransmitImageDataPiece()
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
vtkTransmitImageDataPiece::~vtkTransmitImageDataPiece()
{
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
int vtkTransmitImageDataPiece::RequestInformation(
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
    double spacing[3];
    double origin[3];

    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    if (this->Controller->GetLocalProcessId() == 0)
      {
      //Root sends meta-information to the satellites.

      vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExtent);

      vtkImageData *input = vtkImageData::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
      input->GetDimensions(dims);
      input->GetSpacing(spacing);
      input->GetSpacing(origin);

      int numProcs = this->Controller->GetNumberOfProcesses();
      for (int i = 1; i < numProcs; ++i)
        {
        this->Controller->Send(wExtent, 6, i, 22342);
        this->Controller->Send(dims, 3, i, 22342);
        this->Controller->Send(spacing, 3, i, 22342);
        this->Controller->Send(origin, 3, i, 22342);
        }
      }
    else
      {
      //Satellites ask root for meta-info, because they do not read it themselves.
      
      this->Controller->Receive(wExtent, 6, 0, 22342);
      this->Controller->Receive(dims, 3, 0, 22342);
      this->Controller->Receive(spacing, 3, 0, 22342);
      this->Controller->Receive(origin, 3, 0, 22342);

      vtkImageData *output = vtkImageData::SafeDownCast(
         outInfo->Get(vtkDataObject::DATA_OBJECT()));

      output->SetExtent(wExtent);
      output->SetDimensions(dims);
      output->SetSpacing(spacing);
      output->SetOrigin(origin);
      }

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                 wExtent,
                 6);

    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkTransmitImageDataPiece::RequestUpdateExtent(
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
    { // Request nothing, get it from root instead.
    }

  return 1;
}

  
//----------------------------------------------------------------------------
int vtkTransmitImageDataPiece::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::SafeDownCast(
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
    vtkImageData *input = vtkImageData::SafeDownCast(
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
void vtkTransmitImageDataPiece::RootExecute(vtkImageData *input,
                                                   vtkImageData *output,
                                                   vtkInformation *outInfo)
{
  vtkImageData *tmp = vtkImageData::New();
  vtkImageClip *extract = vtkImageClip::New();
  extract->ClipDataOn();

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
void vtkTransmitImageDataPiece::SatelliteExecute(
  int, vtkImageData *output, vtkInformation *outInfo)
{
  vtkImageData *tmp = vtkImageData::New();

  //decide what we want to ask for and ask root for it
  int uExtent[7];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExtent);
  uExtent[6] = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  this->Controller->Send(uExtent, 7, 0, 22341);

  int wExtent[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExtent);

  //recieve root's response
  this->Controller->Receive(tmp, 0, 22342);

  //recover structure
  int ext[6];
  tmp->GetExtent(ext);
  output->SetExtent(wExtent);

  //copy in retrieved attributes from sent region
  int usizek = uExtent[5]-uExtent[4]+1;
  int usizej = uExtent[3]-uExtent[2]+1;
  int usizei = uExtent[1]-uExtent[0]+1;
  int usize  = usizek*usizej*usizei; 

  vtkPointData *ipd = tmp->GetPointData();  
  vtkPointData *opd = output->GetPointData();
  opd->CopyAllocate(ipd, usize, 1000);

  vtkCellData *icd = tmp->GetCellData();
  vtkCellData *ocd = output->GetCellData();
  ocd->CopyAllocate(icd, usize, 1000);

  vtkIdType ptCtr = 0;
  vtkIdType clCtr = 0;
  for (int k = uExtent[4]; k <= uExtent[5]; k++) 
    {
    for (int j = uExtent[2]; j <= uExtent[3]; j++) 
      {
      for (int i = uExtent[0]; i <= uExtent[1]; i++) 
        {
        int ijk[3] = {i,j,k};
        vtkIdType oPointId = output->ComputePointId(ijk);
        opd->CopyData(ipd, ptCtr++, oPointId); 
        vtkIdType oCellId = output->ComputeCellId(ijk);
        ocd->CopyData(icd, clCtr++, oCellId);
        }
      }        
    }

  //copy in retrieved field data
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
void vtkTransmitImageDataPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");
  
  os << indent << "Controller: (" << this->Controller << ")\n";

}
