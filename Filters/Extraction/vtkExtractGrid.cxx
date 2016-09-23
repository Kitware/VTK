/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractGrid.h"

#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkExtractStructuredGridHelper.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"

vtkStandardNewMacro(vtkExtractGrid);

// Construct object to extract all of the input data.
vtkExtractGrid::vtkExtractGrid()
{
  this->VOI[0] = this->VOI[2] = this->VOI[4] = 0;
  this->VOI[1] = this->VOI[3] = this->VOI[5] = VTK_INT_MAX;

  this->SampleRate[0] = this->SampleRate[1] = this->SampleRate[2] = 1;
  this->IncludeBoundary = 0;
  this->Internal = vtkExtractStructuredGridHelper::New();
}

vtkExtractGrid::~vtkExtractGrid()
{
  if( this->Internal != NULL )
  {
    this->Internal->Delete();
  }
}

//------------------------------------------------------------------------------
int vtkExtractGrid::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int wholeExtent[6], outWholeExt[6];

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);

  this->Internal->Initialize(
      this->VOI,wholeExtent,this->SampleRate,(this->IncludeBoundary==1));

  if (!this->Internal->IsValid())
  {
    vtkDebugMacro("Error while initializing filter.");
    return 0;
  }

  this->Internal->GetOutputWholeExtent(outWholeExt);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               outWholeExt, 6);
  return 1;
}

int vtkExtractGrid::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (!this->Internal->IsValid())
  {
    return 0;
  }

  int i;

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  bool emptyExtent = false;
  int uExt[6];
  for (i=0; i<3; i++)
  {
    if (this->Internal->GetSize(i) < 1)
    {
      uExt[0] = uExt[2] = uExt[4] = 0;
      uExt[1] = uExt[3] = uExt[5] = -1;
      emptyExtent = true;
      break;
    }
  }

  if (!emptyExtent)
  {
    // Find input update extent based on requested output
    // extent
    int oUExt[6];
    outputVector->GetInformationObject(0)->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), oUExt);
    int oWExt[6]; // For parallel parititon this will be different.
    this->Internal->GetOutputWholeExtent(oWExt);
    for (i=0; i<3; i++)
    {
      int idx = oUExt[2*i] - oWExt[2*i]; // Extent value to index
      if (idx < 0 || idx >= (int)this->Internal->GetSize(i))
      {
        vtkWarningMacro("Requested extent outside whole extent.")
        idx = 0;
      }
      uExt[2*i] = this->Internal->GetMappedExtentValueFromIndex(i, idx);
      int jdx = oUExt[2*i+1] - oWExt[2*i]; // Extent value to index
      if (jdx < idx || jdx >= (int)this->Internal->GetSize(i))
      {
        vtkWarningMacro("Requested extent outside whole extent.")
        jdx = 0;
      }
      uExt[2*i + 1] = this->Internal->GetMappedExtentValueFromIndex(i, jdx);
    }
  }
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt, 6);
  // We can handle anything.
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 0);

  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractGrid::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // Reset internal helper to the actual extents of the piece we're working on:
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkStructuredGrid *inGrid = vtkStructuredGrid::GetData(inInfo);
  this->Internal->Initialize(this->VOI, inGrid->GetExtent(), this->SampleRate,
                             (this->IncludeBoundary != 0));

  if (!this->Internal->IsValid())
  {
    return 0;
  }

  // Set the output extent -- this is how RequestDataImpl knows what to copy.
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkStructuredGrid *output = vtkStructuredGrid::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT()));
  output->SetExtent(this->Internal->GetOutputWholeExtent());

  return this->RequestDataImpl(inputVector, outputVector) ? 1 : 0;
}

//------------------------------------------------------------------------------
bool vtkExtractGrid::RequestDataImpl(vtkInformationVector **inputVector,
                                     vtkInformationVector *outputVector)
{
  if( (this->SampleRate[0] < 1) ||
      (this->SampleRate[1] < 1) ||
      (this->SampleRate[2] < 1) )
  {
    vtkErrorMacro("SampleRate must be >= 1 in all 3 dimensions!");
    return false;
  }

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkStructuredGrid *input = vtkStructuredGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkStructuredGrid *output = vtkStructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input->GetNumberOfPoints() == 0)
  {
    return true;
  }

  vtkPointData *pd=input->GetPointData();
  vtkCellData *cd=input->GetCellData();
  vtkPointData *outPD=output->GetPointData();
  vtkCellData *outCD=output->GetCellData();

  vtkPoints *inPts = input->GetPoints();
  int *inExt = input->GetExtent();

  vtkPoints *newPts = inPts->NewInstance();
  int *outExt = output->GetExtent();

  vtkDebugMacro(<< "Extracting Grid");

  this->Internal->CopyPointsAndPointData(inExt,outExt,pd,inPts,outPD,newPts);
  output->SetPoints(newPts);
  newPts->Delete();

  this->Internal->CopyCellData(inExt,outExt,cd,outCD);

  return true;
}

void vtkExtractGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VOI: \n";
  os << indent << "  Imin,Imax: (" << this->VOI[0] << ", "
     << this->VOI[1] << ")\n";
  os << indent << "  Jmin,Jmax: (" << this->VOI[2] << ", "
     << this->VOI[3] << ")\n";
  os << indent << "  Kmin,Kmax: (" << this->VOI[4] << ", "
     << this->VOI[5] << ")\n";

  os << indent << "Sample Rate: (" << this->SampleRate[0] << ", "
               << this->SampleRate[1] << ", "
               << this->SampleRate[2] << ")\n";

  os << indent << "Include Boundary: "
     << (this->IncludeBoundary ? "On\n" : "Off\n");
}
