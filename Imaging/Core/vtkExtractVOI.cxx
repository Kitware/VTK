/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVOI.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractVOI.h"

#include "vtkCellData.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkExtractVOI);

// Construct object to extract all of the input data.
vtkExtractVOI::vtkExtractVOI()
{
  this->VOI[0] = this->VOI[2] = this->VOI[4] = 0;
  this->VOI[1] = this->VOI[3] = this->VOI[5] = VTK_LARGE_INTEGER;

  this->SampleRate[0] = this->SampleRate[1] = this->SampleRate[2] = 1;
}

int vtkExtractVOI::RequestUpdateExtent(
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
    {
    ext[1] = voi[1];
    }
  ext[2] = voi[2] + (updateExt[2]-outWholeExt[2])*rate[1];
  ext[3] = voi[2] + (updateExt[3]-outWholeExt[2])*rate[1];
  if (ext[3] > voi[3])
    {
    ext[3] = voi[3];
    }
  ext[4] = voi[4] + (updateExt[4]-outWholeExt[4])*rate[2];
  ext[5] = voi[4] + (updateExt[5]-outWholeExt[4])*rate[2];
  if (ext[5] > voi[5])
    {
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

int vtkExtractVOI::RequestInformation(
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

  double spacing[3], outSpacing[3];
  inInfo->Get(vtkDataObject::SPACING(), spacing);

  double origin[3], outOrigin[3];
  inInfo->Get(vtkDataObject::ORIGIN(), origin );

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
    mins[i] = static_cast<int>(floor(static_cast<float>(voi[2*i])
                                     / static_cast<float>(rate[i])));

    outSpacing[i] = spacing[i] * rate[i];
    outOrigin[i] = origin[i] + voi[2*i]*spacing[i]-mins[i]*outSpacing[i];
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
  outInfo->Set(vtkDataObject::SPACING(), outSpacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), outOrigin, 3);

  return 1;
}

int vtkExtractVOI::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData *input = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointData *pd=input->GetPointData();
  vtkCellData *cd=input->GetCellData();
  vtkPointData *outPD=output->GetPointData();
  vtkCellData *outCD=output->GetCellData();
  vtkIdType i, j, k, uExt[6], voi[6];
  int uExt32[6];

  const vtkTypeInt32* inExt32;
  vtkIdType inExt[6];

  const vtkTypeInt32* inWholeExt32;
  vtkIdType inWholeExt[6];

  vtkIdType iIn, jIn, kIn;
  vtkIdType outSize, jOffset, kOffset, rate[3];
  vtkIdType idx, newIdx, newCellId;
  vtkIdType inInc1, inInc2;
  // Function to convert output index to input index f(i) = Rate*I + shift
  vtkIdType shift[3];

  vtkDebugMacro(<< "Extracting Grid");

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt32);
  uExt[0] = uExt32[0]; uExt[1] = uExt32[1]; uExt[2] = uExt32[2];
  uExt[3] = uExt32[3]; uExt[4] = uExt32[4]; uExt[5] = uExt32[5];

  inExt32 = input->GetExtent();
  inExt[0] = inExt32[0]; inExt[1] = inExt32[1]; inExt[2] = inExt32[2];
  inExt[3] = inExt32[3]; inExt[4] = inExt32[4]; inExt[5] = inExt32[5];
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
  inWholeExt32 = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  inWholeExt[0] = inWholeExt32[0];
  inWholeExt[1] = inWholeExt32[1];
  inWholeExt[2] = inWholeExt32[2];
  inWholeExt[3] = inWholeExt32[3];
  inWholeExt[4] = inWholeExt32[4];
  inWholeExt[5] = inWholeExt32[5];

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
  shift[0] = static_cast<vtkIdType>(
    floor(static_cast<float>(voi[0])/static_cast<float>(rate[0]) ));
  shift[1] = static_cast<vtkIdType>(
    floor( static_cast<float>(voi[2])/static_cast<float>(rate[1]) ));
  shift[2] = static_cast<vtkIdType>(
    floor( static_cast<float>(voi[4])/static_cast<float>(rate[2]) ));
  // Take the different between the output and input mins (in input coordinates).
  shift[0] = voi[0] - (shift[0]*rate[0]);
  shift[1] = voi[2] - (shift[1]*rate[1]);
  shift[2] = voi[4] - (shift[2]*rate[2]);

  output->SetExtent(uExt32);

  // If output same as input, just pass data through
  if ( uExt[0] <= inExt[0] && uExt[1] >= inExt[1] &&
       uExt[2] <= inExt[2] && uExt[3] >= inExt[3] &&
       uExt[4] <= inExt[4] && uExt[5] >= inExt[5] &&
       rate[0] == 1 && rate[1] == 1 && rate[2] == 1)
    {
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

  // Traverse input data and copy point attributes to output
  // iIn,jIn,kIn are in input grid coordinates.
  newIdx = 0;
  for ( k=uExt[4]; k <= uExt[5]; ++k)
    { // Convert out coords to in coords.
    kIn = shift[2] + (k*rate[2]);
    if (kIn > voi[5])
      {
      kIn = voi[5];
      }
    kOffset = (kIn-inExt[4]) * inInc2;
    for ( j=uExt[2]; j <= uExt[3]; ++j)
      { // Convert out coords to in coords.
      jIn = shift[1] + (j*rate[1]);
      if (jIn > voi[3])
        {
        jIn = voi[3];
        }
      jOffset = (jIn-inExt[2]) * inInc1;
      for ( i=uExt[0]; i <= uExt[1]; ++i)
        { // Convert out coords to in coords.
        iIn = shift[0] + (i*rate[0]);
        if (iIn > voi[1])
          {
          iIn = voi[1];
          }
        idx = (iIn-inExt[0]) + jOffset + kOffset;
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

  return 1;
}

void vtkExtractVOI::PrintSelf(ostream& os, vtkIndent indent)
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

}
