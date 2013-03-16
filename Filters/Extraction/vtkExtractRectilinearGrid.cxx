/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractRectilinearGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractRectilinearGrid.h"

#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkExtractRectilinearGrid);

// Construct object to extract all of the input data.
vtkExtractRectilinearGrid::vtkExtractRectilinearGrid()
{
  this->VOI[0] = this->VOI[2] = this->VOI[4] = 0;
  this->VOI[1] = this->VOI[3] = this->VOI[5] = VTK_INT_MAX;

  this->SampleRate[0] = this->SampleRate[1] = this->SampleRate[2] = 1;

  this->IncludeBoundary = 0;
}

//----------------------------------------------------------------------------
int vtkExtractRectilinearGrid::RequestUpdateExtent(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int i, ext[6], voi[6];
  int *inWholeExt, *outWholeExt, *updateExt;
  int rate[3];


  inWholeExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  outWholeExt = outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  updateExt = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  for (i = 0; i < 3; ++i)
    {
    rate[i] = this->SampleRate[i];
    if (rate[i] < 1)
      {
      rate[i] = 1;
      }
    }

  // Once again, clip the VOI with the input whole extent.
  for (i = 0; i < 3; ++i)
    {
    voi[i*2] = this->VOI[2*i];
    if (voi[2*i] < inWholeExt[2*i])
      {
      voi[2*i] = inWholeExt[2*i];
      }
    voi[i*2+1] = this->VOI[2*i+1];
    if (voi[2*i+1] > inWholeExt[2*i+1])
      {
      voi[2*i+1] = inWholeExt[2*i+1];
      }
    }

  ext[0] = voi[0] + (updateExt[0]-outWholeExt[0])*rate[0];
  ext[1] = voi[0] + (updateExt[1]-outWholeExt[0])*rate[0];
  if (ext[1] > voi[1])
    { // This handles the IncludeBoundary condition.
    ext[1] = voi[1];
    }
  ext[2] = voi[2] + (updateExt[2]-outWholeExt[2])*rate[1];
  ext[3] = voi[2] + (updateExt[3]-outWholeExt[2])*rate[1];
  if (ext[3] > voi[3])
    { // This handles the IncludeBoundary condition.
    ext[3] = voi[3];
    }
  ext[4] = voi[4] + (updateExt[4]-outWholeExt[4])*rate[2];
  ext[5] = voi[4] + (updateExt[5]-outWholeExt[4])*rate[2];
  if (ext[5] > voi[5])
    { // This handles the IncludeBoundary condition.
    ext[5] = voi[5];
    }

  // I do not think we need this extra check, but it cannot hurt.
  if (ext[0] < inWholeExt[0])
    {
    ext[0] = inWholeExt[0];
    }
  if (ext[1] > inWholeExt[1])
    {
    ext[1] = inWholeExt[1];
    }

  if (ext[2] < inWholeExt[2])
    {
    ext[2] = inWholeExt[2];
    }
  if (ext[3] > inWholeExt[3])
    {
    ext[3] = inWholeExt[3];
    }

  if (ext[4] < inWholeExt[4])
    {
    ext[4] = inWholeExt[4];
    }
  if (ext[5] > inWholeExt[5])
    {
    ext[5] = inWholeExt[5];
    }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
  // We can handle anything.
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 0);

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractRectilinearGrid::RequestInformation(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int i, outDims[3], voi[6];
  int inWholeExtent[6], outWholeExtent[6];
  int mins[3];
  int rate[3];

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inWholeExtent);

  // Copy because we need to take union of voi and whole extent.
  for ( i=0; i < 6; i++ )
    {
    voi[i] = this->VOI[i];
    }

  for ( i=0; i < 3; i++ )
    {
    // Empty request.
    if (voi[2*i+1] < voi[2*i] || voi[2*i+1] < inWholeExtent[2*i] ||
        voi[2*i] > inWholeExtent[2*i+1])
      {
      outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                   0,-1,0,-1,0,-1);
      return 1;
      }

    // Make sure VOI is in the whole extent.
    if ( voi[2*i+1] > inWholeExtent[2*i+1] )
      {
      voi[2*i+1] = inWholeExtent[2*i+1];
      }
    else if ( voi[2*i+1] < inWholeExtent[2*i] )
      {
      voi[2*i+1] = inWholeExtent[2*i];
      }
    if ( voi[2*i] > inWholeExtent[2*i+1] )
      {
      voi[2*i] = inWholeExtent[2*i+1];
      }
    else if ( voi[2*i] < inWholeExtent[2*i] )
      {
      voi[2*i] = inWholeExtent[2*i];
      }

    if ( (rate[i] = this->SampleRate[i]) < 1 )
      {
      rate[i] = 1;
      }

    outDims[i] = (voi[2*i+1] - voi[2*i]) / rate[i] + 1;
    if ( outDims[i] < 1 )
      {
      outDims[i] = 1;
      }
    // We might as well make this work for negative extents.
    mins[i] = static_cast<int>(floor(voi[2*i]/static_cast<double>(rate[i])));
    }

  // Adjust the output dimensions if the boundaries are to be
  // included and the sample rate is not 1.
  if ( this->IncludeBoundary &&
       (rate[0] != 1 || rate[1] != 1 || rate[2] != 1) )
    {
    int diff;
    for (i=0; i<3; i++)
      {
      if ( ((diff=voi[2*i+1]-voi[2*i]) > 0) && rate[i] != 1 &&
           ((diff % rate[i]) != 0) )
        {
        outDims[i]++;
        }
      }
    }

  // Set the whole extent of the output
  outWholeExtent[0] = mins[0];
  outWholeExtent[1] = mins[0] + outDims[0] - 1;
  outWholeExtent[2] = mins[1];
  outWholeExtent[3] = mins[1] + outDims[1] - 1;
  outWholeExtent[4] = mins[2];
  outWholeExtent[5] = mins[2] + outDims[2] - 1;

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               outWholeExtent, 6);

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractRectilinearGrid::RequestData(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkRectilinearGrid *input= vtkRectilinearGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointData *pd=input->GetPointData();
  vtkCellData *cd=input->GetCellData();
  vtkRectilinearGrid *output= vtkRectilinearGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointData *outPD=output->GetPointData();
  vtkCellData *outCD=output->GetCellData();
  int uExt[6], voi[6];
  int *inExt, *outWholeExt;
  vtkIdType i, j, k;
  vtkIdType iIn, jIn, kIn;
  int outSize, jOffset, kOffset, rate[3];
  vtkIdType idx, newIdx, newCellId;
  int inInc1, inInc2;
  int *inWholeExt;

  vtkDebugMacro(<< "Extracting Grid");

  inWholeExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  outWholeExt = outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt);
  inExt = input->GetExtent();
  inInc1 = (inExt[1]-inExt[0]+1);
  inInc2 = inInc1*(inExt[3]-inExt[2]+1);

  for (i = 0; i < 3; ++i)
    {
    if ( (rate[i] = this->SampleRate[i]) < 1 )
      {
      rate[i] = 1;
      }
    }

  // Clip the VOI by the input extent
  for (i = 0; i < 3; ++i)
    {
    voi[i*2] = this->VOI[2*i];
    if (voi[2*i] < inWholeExt[2*i])
      {
      voi[2*i] = inWholeExt[2*i];
      }
    voi[i*2+1] = this->VOI[2*i+1];
    if (voi[2*i+1] > inWholeExt[2*i+1])
      {
      voi[2*i+1] = inWholeExt[2*i+1];
      }
    }

  output->SetExtent(uExt);

  // If output same as input, just pass data through
  if ( uExt[0] <= inExt[0] && uExt[1] >= inExt[1] &&
       uExt[2] <= inExt[2] && uExt[3] >= inExt[3] &&
       uExt[4] <= inExt[4] && uExt[5] >= inExt[5] &&
       rate[0] == 1 && rate[1] == 1 && rate[2] == 1)
    {
    output->SetXCoordinates(input->GetXCoordinates());
    output->SetYCoordinates(input->GetYCoordinates());
    output->SetZCoordinates(input->GetZCoordinates());
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    vtkDebugMacro(<<"Passed data through bacause input and output are the same");
    return 1;
    }

  // Allocate necessary objects
  //
  outSize = (uExt[1]-uExt[0]+1)*(uExt[3]-uExt[2]+1)*(uExt[5]-uExt[4]+1);
  outPD->CopyAllocate(pd,outSize,outSize);
  outCD->CopyAllocate(cd,outSize,outSize);

  // Setup the new "geometry"
  vtkDataArray *inCoords;
  vtkFloatArray *outCoords;
  // X
  inCoords = input->GetXCoordinates();
  if (inCoords->GetNumberOfComponents() > 1)
    {
    vtkWarningMacro("Multiple componenet axis coordinate.");
    }
  outCoords = vtkFloatArray::New();
  outCoords->Allocate(uExt[1]-uExt[0]+1);
  outCoords->Allocate(uExt[1]-uExt[0]+1);
  outCoords->SetNumberOfTuples(uExt[1]-uExt[0]+1);
  for ( k=uExt[0]; k <= uExt[1]; ++k)
    { // Convert out coords to in coords.
    kIn = voi[0] + ((k-outWholeExt[0])*rate[0]);
    if (kIn > voi[1])
      { // This handles the IncludeBoundaryOn condition.
      kIn = voi[1];
      }
    outCoords->SetValue(k-uExt[0], inCoords->GetComponent(kIn-inExt[0], 0));
    }
  output->SetXCoordinates(outCoords);
  outCoords->Delete();
  outCoords = NULL;
  // Y
  inCoords = input->GetYCoordinates();
  if (inCoords->GetNumberOfComponents() > 1)
    {
    vtkWarningMacro("Multiple componenet axis coordinate.");
    }
  outCoords = vtkFloatArray::New();
  outCoords->Allocate(uExt[3]-uExt[2]+1);
  outCoords->SetNumberOfTuples(uExt[3]-uExt[2]+1);
  for ( k=uExt[2]; k <= uExt[3]; ++k)
    { // Convert out coords to in coords.
    kIn = voi[2] + ((k-outWholeExt[2])*rate[1]);
    if (kIn > voi[3])
      { // This handles the IncludeBoundaryOn condition.
      kIn = voi[3];
      }
    outCoords->SetValue(k-uExt[2], inCoords->GetComponent(kIn-inExt[2], 0));
    }
  output->SetYCoordinates(outCoords);
  outCoords->Delete();
  // Z
  inCoords = input->GetZCoordinates();
  if (inCoords->GetNumberOfComponents() > 1)
    {
    vtkWarningMacro("Multiple componenet axis coordinate.");
    }
  outCoords = vtkFloatArray::New();
  outCoords->Allocate(uExt[5]-uExt[4]+1);
  outCoords->SetNumberOfTuples(uExt[5]-uExt[4]+1);
  for ( k=uExt[4]; k <= uExt[5]; ++k)
    { // Convert out coords to in coords.
    kIn = voi[4] + ((k-outWholeExt[4])*rate[2]);
    if (kIn > voi[5])
      { // This handles the IncludeBoundaryOn condition.
      kIn = voi[5];
      }
    outCoords->SetValue(k-uExt[4], inCoords->GetComponent(kIn-inExt[4], 0));
    }
  output->SetZCoordinates(outCoords);
  outCoords->Delete();

  // Traverse input data and copy point attributes to output
  // iIn,jIn,kIn are in input grid coordinates.
  newIdx = 0;
  for ( k=uExt[4]; k <= uExt[5]; ++k)
    { // Convert out coords to in coords.
    kIn = voi[4] + ((k-outWholeExt[4])*rate[2]);
    if (kIn > voi[5])
      { // This handles the IncludeBoundaryOn condition.
      kIn = voi[5];
      }
    kOffset = (kIn-inExt[4]) * inInc2;
    for ( j=uExt[2]; j <= uExt[3]; ++j)
      { // Convert out coords to in coords.
      jIn = voi[2] + ((j-outWholeExt[2])*rate[1]);
      if (jIn > voi[3])
        { // This handles the IncludeBoundaryOn condition.
        jIn = voi[3];
        }
      jOffset = (jIn-inExt[2]) * inInc1;
      for ( i=uExt[0]; i <= uExt[1]; ++i)
        { // Convert out coords to in coords.
        iIn = voi[0] + ((i-outWholeExt[0])*rate[0]);
        if (iIn > voi[1])
          { // This handles the IncludeBoundaryOn condition.
          iIn = voi[1];
          }
        idx = (iIn-inExt[0]) + jOffset + kOffset;
        //newPts->SetPoint(newIdx,inPts->GetPoint(idx));
        outPD->CopyData(pd, idx, newIdx++);
        }
      }
    }

  // Traverse input data and copy cell attributes to output
  //
  newCellId = 0;
  inInc1 = (inExt[1]-inExt[0]);
  inInc2 = inInc1*(inExt[3]-inExt[2]);
  // This will take care of 2D and 1D cells.
  // Each loop has to excute at least once.
  if (uExt[4] == uExt[5])
    {
    uExt[5] = uExt[5] + 1;
    }
  if (uExt[2] == uExt[3])
    {
    uExt[3] = uExt[3] + 1;
    }
  if (uExt[0] == uExt[1])
    {
    uExt[1] = uExt[1] + 1;
    }
  // No need to consider IncludeBoundary for cell data.
  for ( k=uExt[4]; k < uExt[5]; ++k )
    { // Convert out coords to in coords.
    kIn = voi[4] + ((k-outWholeExt[4])*rate[2]);
    kOffset = (kIn-inExt[4]) * inInc2;
    for ( j=uExt[2]; j < uExt[3]; ++j )
      { // Convert out coords to in coords.
      jIn = voi[2] + ((j-outWholeExt[2])*rate[1]);
      jOffset = (jIn-inExt[2]) * inInc1;
      for ( i=uExt[0]; i < uExt[1]; ++i )
        {
        iIn = voi[0] + ((i-outWholeExt[0])*rate[0]);
        idx = (iIn-inExt[0]) + jOffset + kOffset;
        outCD->CopyData(cd, idx, newCellId++);
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractRectilinearGrid::PrintSelf(ostream& os, vtkIndent indent)
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
