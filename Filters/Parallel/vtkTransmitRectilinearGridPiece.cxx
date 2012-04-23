/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitRectilinearGridPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransmitRectilinearGridPiece.h"

#include "vtkCellData.h"
#include "vtkExtractRectilinearGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkRectilinearGrid.h"
#include "vtkDoubleArray.h"

vtkStandardNewMacro(vtkTransmitRectilinearGridPiece);

vtkCxxSetObjectMacro(vtkTransmitRectilinearGridPiece,Controller,
                     vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkTransmitRectilinearGridPiece::vtkTransmitRectilinearGridPiece()
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
vtkTransmitRectilinearGridPiece::~vtkTransmitRectilinearGridPiece()
{
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
int vtkTransmitRectilinearGridPiece::RequestInformation(
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

    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    if (this->Controller->GetLocalProcessId() == 0)
      {
      //Root sends meta-information to the satellites.
      vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExtent);

      int numProcs = this->Controller->GetNumberOfProcesses();
      for (int i = 1; i < numProcs; ++i)
        {
        this->Controller->Send(wExtent, 6, i, 22342);
        }
      }
    else
      {
      //Satellites ask root for meta-info, because they do not read it themselves.
      this->Controller->Receive(wExtent, 6, 0, 22342);

      vtkRectilinearGrid *output = vtkRectilinearGrid::SafeDownCast(
         outInfo->Get(vtkDataObject::DATA_OBJECT()));
      output->SetExtent(wExtent);
      }

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                 wExtent,
                 6);
    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkTransmitRectilinearGridPiece::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info object
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
int vtkTransmitRectilinearGridPiece::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkRectilinearGrid *output = vtkRectilinearGrid::SafeDownCast(
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
    vtkRectilinearGrid *input = vtkRectilinearGrid::SafeDownCast(
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
    int updatePiece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    int updateNumPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    int* wholeExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    vtkExtentTranslator* et = vtkStreamingDemandDrivenPipeline::GetExtentTranslator(inInfo);
    output->GenerateGhostLevelArray(updatePiece,
                                    updateNumPieces,
                                    ghostLevel,
                                    wholeExt,
                                    et);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkTransmitRectilinearGridPiece::RootExecute(vtkRectilinearGrid *input,
                                                   vtkRectilinearGrid *output,
                                                   vtkInformation *outInfo)
{
  vtkRectilinearGrid *tmp = vtkRectilinearGrid::New();
  vtkExtractRectilinearGrid *extract = vtkExtractRectilinearGrid::New();
  int ext[7];
  int numProcs, i;

  int outExtent[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExtent);

  vtkStreamingDemandDrivenPipeline *extractExecutive =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(extract->GetExecutive());

  // First, set up the pipeline and handle local request.
  tmp->ShallowCopy(input);
  extract->SetInputData(tmp);
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
void vtkTransmitRectilinearGridPiece::SatelliteExecute(
  int, vtkRectilinearGrid *output, vtkInformation *outInfo)
{
  vtkRectilinearGrid *tmp = vtkRectilinearGrid::New();

  //decide what we want to ask for and ask root for it
  int uExtent[7];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExtent);
  uExtent[6] = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  this->Controller->Send(uExtent, 7, 0, 22341);

  int wExtent[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExtent);

  //receive root's response
  this->Controller->Receive(tmp, 0, 22342);

  //recover structure
  int ext[6];
  tmp->GetExtent(ext);

  output->SetExtent(wExtent);

  //create coordinate array of whole size, but only fill in
  //out piece with what root sent to us
  vtkDataArray *ida = tmp->GetZCoordinates();
  vtkDataArray *oda = ida->NewInstance();
  oda->SetNumberOfComponents(ida->GetNumberOfComponents());
  oda->SetNumberOfTuples(wExtent[5]-wExtent[4]+1);
  for (int i = uExtent[4]; i <= uExtent[5]; i++)
    {
    oda->SetTuple(i, ida->GetTuple(i-uExtent[4]));
    }
  output->SetZCoordinates(oda);
  oda->Delete();

  ida = tmp->GetYCoordinates();
  oda = ida->NewInstance();
  oda->SetNumberOfComponents(ida->GetNumberOfComponents());
  oda->SetNumberOfTuples(wExtent[3]-wExtent[2]+1);
  for (int i = uExtent[2]; i <= uExtent[3]; i++)
    {
    oda->SetTuple(i, ida->GetTuple(i-uExtent[2]));
    }
  output->SetYCoordinates(oda);
  oda->Delete();

  ida = tmp->GetXCoordinates();
  oda = ida->NewInstance();
  oda->SetNumberOfComponents(ida->GetNumberOfComponents());
  oda->SetNumberOfTuples(wExtent[1]-wExtent[0]+1);
  for (int i = uExtent[0]; i <= uExtent[1]; i++)
    {
    oda->SetTuple(i, ida->GetTuple(i-uExtent[0]));
    }
  output->SetXCoordinates(oda);
  oda->Delete();

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
void vtkTransmitRectilinearGridPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");

  os << indent << "Controller: (" << this->Controller << ")\n";

}

