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

vtkCxxRevisionMacro(vtkExtractVOI, "1.42");
vtkStandardNewMacro(vtkExtractVOI);

//-----------------------------------------------------------------------------
// Construct object to extract all of the input data.
vtkExtractVOI::vtkExtractVOI()
{
  this->VOI[0] = this->VOI[2] = this->VOI[4] = -VTK_LARGE_INTEGER;
  this->VOI[1] = this->VOI[3] = this->VOI[5] = VTK_LARGE_INTEGER;

  this->SampleRate[0] = this->SampleRate[1] = this->SampleRate[2] = 1;
}

//-----------------------------------------------------------------------------
// Get ALL of the input.
void vtkExtractVOI::RequestUpdateExtent (
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed( outputVector ))
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // request all of the VOI, (note from Ken why are we not looking at the UE?)
  int i;
  int inExt[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),inExt);

  // no need to go outside the VOI
  for (i = 0; i < 3; ++i)
    {
    if (inExt[i*2] < this->VOI[i*2])
      {
      inExt[i*2] = this->VOI[i*2];
      }
    if (inExt[i*2+1] > this->VOI[i*2+1])
      {
      inExt[i*2+1] = this->VOI[i*2+1];
      }
    }
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inExt,6);
}

//-----------------------------------------------------------------------------
void 
vtkExtractVOI::RequestInformation (
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  int i, outDims[3], voi[6];
  int rate[3];
  int wholeExtent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent );
  
  double ar[3], outAR[3];
  inInfo->Get(vtkDataObject::SPACING(), ar );
  
  double origin[3], outOrigin[3];
  inInfo->Get(vtkDataObject::ORIGIN(), origin );
  
  for ( i=0; i < 6; i++ )
    {
    voi[i] = this->VOI[i];
    }

  for ( i=0; i < 3; i++ )
    {
    if ( voi[2*i+1] > wholeExtent[2*i+1] )
      {
      voi[2*i+1] = wholeExtent[2*i+1];
      }
    else if ( voi[2*i+1] < wholeExtent[2*i] )
      {
      voi[2*i+1] = wholeExtent[2*i];
      }
    if ( voi[2*i] < wholeExtent[2*i] )
      {
      voi[2*i] = wholeExtent[2*i];
      }
    else if ( voi[2*i] > wholeExtent[2*i+1] )
      {
      voi[2*i] = wholeExtent[2*i+1];
      }

    if ( voi[2*i] > voi[2*i+1] )
      {
      voi[2*i] = voi[2*i+1];
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
    outAR[i] = ar[i] * this->SampleRate[i];
    wholeExtent[i*2] = voi[i*2];
    wholeExtent[i*2+1] = voi[i*2] + outDims[i] - 1;
    outOrigin[i] = origin[i] + voi[2*i]*ar[i] - wholeExtent[2*i]*outAR[i];
    }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent ,6);
  outInfo->Set(vtkDataObject::SPACING(), outAR, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), outOrigin, 3);
}

//-----------------------------------------------------------------------------
void vtkExtractVOI::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the data objects
  vtkInformation *outInfo = 
    outputVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageData *input = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkPointData *pd=input->GetPointData();
  vtkCellData *cd=input->GetCellData();
  output->SetExtent(output->GetWholeExtent());
  vtkPointData *outPD=output->GetPointData();
  vtkCellData *outCD=output->GetCellData();
  int i, j, k, dims[3], outDims[3], voi[6], dim, idx, newIdx;
  int newCellId;
  double origin[3], ar[3];
  int sliceSize, outSize, jOffset, kOffset, rate[3];
  int wholeExtent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExtent);
  int *inExt = input->GetExtent();

  vtkDebugMacro(<< "Extracting VOI");
  //
  // Check VOI and clamp as necessary. Compute output parameters,
  //
  input->GetDimensions(dims);
  input->GetOrigin(origin);
  input->GetSpacing(ar);

  for ( i=0; i < 6; i++ )
    {
    voi[i] = this->VOI[i];
    }

  for ( outSize=1, dim=0, i=0; i < 3; i++ )
    {
    if ( voi[2*i+1] > wholeExtent[2*i+1] )
      {
      voi[2*i+1] = wholeExtent[2*i+1];
      }
    else if ( voi[2*i+1] < wholeExtent[2*i] )
      {
      voi[2*i+1] = wholeExtent[2*i];
      }
    if ( voi[2*i] < wholeExtent[2*i] )
      {
      voi[2*i] = wholeExtent[2*i];
      }
    else if ( voi[2*i] > wholeExtent[2*i+1] )
      {
      voi[2*i] = wholeExtent[2*i+1];
      }

    if ( voi[2*i] > voi[2*i+1] )
      {
      voi[2*i] = voi[2*i+1];
      }

    if ( (voi[2*i+1]-voi[2*i]) > 0 )
      {
      dim++;
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

    outSize *= outDims[i];
    }
  
  // 
  // If output same as input, just pass data through
  //
  if ( outDims[0] == dims[0] && outDims[1] == dims[1] && outDims[2] == dims[2] &&
       rate[0] == 1 && rate[1] == 1 && rate[2] == 1 )
    {
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    vtkDebugMacro(<<"Passed data through because input and output are the same");
    return;
    }
  //
  // Allocate necessary objects
  //
  outPD->CopyAllocate(pd,outSize,outSize);
  outCD->CopyAllocate(cd,outSize,outSize);
  sliceSize = dims[0]*dims[1];
  
  //
  // Traverse input data and copy point attributes to output
  //
  newIdx = 0;
  for ( k=voi[4]; k <= voi[5]; k += rate[2] )
    {
    kOffset = (k-inExt[4]) * sliceSize;
    for ( j=voi[2]; j <= voi[3]; j += rate[1] )
      {
      jOffset = (j-inExt[2]) * dims[0];
      for ( i=voi[0]; i <= voi[1]; i += rate[0] )
        {
        idx = (i-inExt[0]) + jOffset + kOffset;
        outPD->CopyData(pd, idx, newIdx++);
        }
      }
    }

  //
  // Traverse input data and copy cell attributes to output
  //
  // Handle 2D, 1D and 0D degenerate data sets.
  if (voi[5] == voi[4])
    {
    ++voi[5];
    }
  if (voi[3] == voi[2])
    {
    ++voi[3];
    }
  if (voi[1] == voi[0])
    {
    ++voi[1];
    }

  newCellId = 0;
  sliceSize = (dims[0]-1)*(dims[1]-1);
  for ( k=voi[4]; k < voi[5]; k += rate[2] )
    {
    kOffset = (k-inExt[4]) * sliceSize;
    for ( j=voi[2]; j < voi[3]; j += rate[1] )
      {
      jOffset = (j-inExt[2]) * (dims[0] - 1);
      for ( i=voi[0]; i < voi[1]; i += rate[0] )
        {
        idx = (i-inExt[0]) + jOffset + kOffset;
        outCD->CopyData(cd, idx, newCellId++);
        }
      }
    }
  
  vtkDebugMacro(<<"Extracted " << newIdx << " point attributes on "
  << dim << "-D dataset\n\tDimensions are (" << outDims[0]
  << "," << outDims[1] << "," << outDims[2] <<")");
}


//-----------------------------------------------------------------------------
void vtkExtractVOI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VOI: \n";
  os << indent << "  Imin,Imax: (" << this->VOI[0] 
     << ", " << this->VOI[1] << ")\n";
  os << indent << "  Jmin,Jmax: (" << this->VOI[2] 
     << ", " << this->VOI[3] << ")\n";
  os << indent << "  Kmin,Kmax: (" << this->VOI[4] 
     << ", " << this->VOI[5] << ")\n";

  os << indent << "Sample Rate: (" << this->SampleRate[0] << ", "
               << this->SampleRate[1] << ", "
               << this->SampleRate[2] << ")\n";
}




