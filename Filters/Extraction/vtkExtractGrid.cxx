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

#include "vtkCellData.h"
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
  this->VOI[1] = this->VOI[3] = this->VOI[5] = VTK_LARGE_INTEGER;

  this->SampleRate[0] = this->SampleRate[1] = this->SampleRate[2] = 1;
  
  this->IncludeBoundary = 0;
}

int vtkExtractGrid::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int i, ext[6], voi[6];
  int *inWholeExt, *outWholeExt, *updateExt;
  int rate[3];

  inWholeExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  // Temporary fix for multi-block datasets. If the WHOLE_EXTENT is not
  // defined, exit gracefully instead of crashing.
  if (!inWholeExt)
    {
    return 1;
    }
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

int vtkExtractGrid::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int i, outDims[3], voi[6], wholeExtent[6];
  int mins[3];
  int rate[3];

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);

  // Copy because we need to take union of voi and whole extent.
  for ( i=0; i < 6; i++ )
    {
    voi[i] = this->VOI[i];
    }

  for ( i=0; i < 3; i++ )
    {
    // Empty request.
    if (voi[2*i+1] < voi[2*i] || voi[2*i+1] < wholeExtent[2*i] || 
        voi[2*i] > wholeExtent[2*i+1])
      {
      outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                   0,-1,0,-1,0,-1);
      return 1;
      }

    // Make sure VOI is in the whole extent.
    if ( voi[2*i+1] > wholeExtent[2*i+1] )
      {
      voi[2*i+1] = wholeExtent[2*i+1];
      }
    else if ( voi[2*i+1] < wholeExtent[2*i] )
      {
      voi[2*i+1] = wholeExtent[2*i];
      }
    if ( voi[2*i] > wholeExtent[2*i+1] )
      {
      voi[2*i] = wholeExtent[2*i+1];
      }
    else if ( voi[2*i] < wholeExtent[2*i] )
      {
      voi[2*i] = wholeExtent[2*i];
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
  wholeExtent[0] = mins[0];
  wholeExtent[1] = mins[0] + outDims[0] - 1;
  wholeExtent[2] = mins[1];
  wholeExtent[3] = mins[1] + outDims[1] - 1;
  wholeExtent[4] = mins[2];
  wholeExtent[5] = mins[2] + outDims[2] - 1;

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               wholeExtent, 6);

  return 1;
}

int vtkExtractGrid::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkStructuredGrid *input = vtkStructuredGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkStructuredGrid *output = vtkStructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointData *pd=input->GetPointData();
  vtkCellData *cd=input->GetCellData();
  vtkPointData *outPD=output->GetPointData();
  vtkCellData *outCD=output->GetCellData();
  int i, j, k, uExt[6], voi[6];
  int *inExt;
  int *inWholeExt;
  int iIn, jIn, kIn;
  int outSize, jOffset, kOffset, rate[3];
  vtkIdType idx, newIdx, newCellId;
  vtkPoints *newPts, *inPts;
  int inInc1, inInc2;
  // Function to convert output index to input index f(i) = Rate*I + shift
  int shift[3];

  vtkDebugMacro(<< "Extracting Grid");

  inPts = input->GetPoints();

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

  // Clip the VOI by the input whole extent
  inWholeExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
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

  // Compute the shift.
  // The shift is necessary because the starting VOI may not be on stride boundary.
  // We need to duplicate the computation done in 
  // ExecuteInformtation for the output whole extent.
  // Use shift as temporary variable (output mins).
  shift[0] = static_cast<int>(floor(voi[0]/static_cast<double>(rate[0])));
  shift[1] = static_cast<int>(floor(voi[2]/static_cast<double>(rate[1])));
  shift[2] = static_cast<int>(floor(voi[4]/static_cast<double>(rate[2])));
  // Take the different between the output and input mins (in input coordinates).
  shift[0] = voi[0] - (shift[0]*rate[0]);
  shift[1] = voi[2] - (shift[1]*rate[1]);
  shift[2] = voi[4] - (shift[2]*rate[2]);

  output->SetExtent(uExt);

  // If output same as input, just pass data through
  if ( uExt[0] <= inExt[0] && uExt[1] >= inExt[1] &&
       uExt[2] <= inExt[2] && uExt[3] >= inExt[3] &&
       uExt[4] <= inExt[4] && uExt[5] >= inExt[5] &&
       rate[0] == 1 && rate[1] == 1 && rate[2] == 1)
    { 
    output->SetPoints(inPts);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    vtkDebugMacro(<<"Passed data through bacause input and output are the same");
    return 1;
    }

  // Allocate necessary objects
  //
  outSize = (uExt[1]-uExt[0]+1)*(uExt[3]-uExt[2]+1)*(uExt[5]-uExt[4]+1);
  newPts = inPts->NewInstance(); 
  newPts->SetDataType(inPts->GetDataType());
  newPts->SetNumberOfPoints(outSize);
  outPD->CopyAllocate(pd,outSize,outSize);
  outCD->CopyAllocate(cd,outSize,outSize);

  // Traverse input data and copy point attributes to output
  // iIn,jIn,kIn are in input grid coordinates.
  newIdx = 0;
  for ( k=uExt[4]; k <= uExt[5]; ++k)
    { // Convert out coords to in coords.
    kIn = shift[2] + (k*rate[2]);
    if (kIn > voi[5])
      { // This handles the IncludeBoundaryOn condition.
      kIn = voi[5];
      }
    kOffset = (kIn-inExt[4]) * inInc2;
    for ( j=uExt[2]; j <= uExt[3]; ++j)
      { // Convert out coords to in coords.
      jIn = shift[1] + (j*rate[1]);
      if (jIn > voi[3])
        { // This handles the IncludeBoundaryOn condition.
        jIn = voi[3];
        }
      jOffset = (jIn-inExt[2]) * inInc1;
      for ( i=uExt[0]; i <= uExt[1]; ++i)
        { // Convert out coords to in coords.
        iIn = shift[0] + (i*rate[0]);
        if (iIn > voi[1])
          { // This handles the IncludeBoundaryOn condition.
          iIn = voi[1];
          }
        idx = (iIn-inExt[0]) + jOffset + kOffset;
        newPts->SetPoint(newIdx,inPts->GetPoint(idx));
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
  // Fix the boundary case
  if (uExt[5] > inExt[5] && uExt[4] > inExt[4])
    {
    uExt[4]--;
    uExt[5]--;
    }
  if (uExt[2] == uExt[3])
    {
    uExt[3] = uExt[3] + 1;
    }
  // Fix the boundary case
  if (uExt[3] > inExt[3] && uExt[2] > inExt[2])
    {
    uExt[2]--;
    uExt[3]--;
    }
  if (uExt[0] == uExt[1])
    {
    uExt[1] = uExt[1] + 1;
    }
  // Fix the boundary case
  if (uExt[1] > inExt[1] && uExt[0] > inExt[0])
    {
    uExt[0]--;
    uExt[1]--;
    }
  // No need to consider IncludeBoundary for cell data.
  for ( k=uExt[4]; k < uExt[5]; ++k )
    { // Convert out coords to in coords.
    kIn = shift[2] + (k*rate[2]);
    kOffset = (kIn-inExt[4]) * inInc2;
    for ( j=uExt[2]; j < uExt[3]; ++j )
      { // Convert out coords to in coords.
      jIn = shift[1] + (j*rate[1]);
      jOffset = (jIn-inExt[2]) * inInc1;
      for ( i=uExt[0]; i < uExt[1]; ++i )
        {
        iIn = shift[0] + (i*rate[0]);
        idx = (iIn-inExt[0]) + jOffset + kOffset;
        outCD->CopyData(cd, idx, newCellId++);
        }
      }
    }

  output->SetPoints(newPts);
  newPts->Delete();

  return 1;
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
