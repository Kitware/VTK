/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVOI.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractVOI.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkExtractVOI, "1.30.4.1");
vtkStandardNewMacro(vtkExtractVOI);

//-----------------------------------------------------------------------------
// Construct object to extract all of the input data.
vtkExtractVOI::vtkExtractVOI()
{
  this->VOI[0] = this->VOI[2] = this->VOI[4] = 0;
  this->VOI[1] = this->VOI[3] = this->VOI[5] = VTK_LARGE_INTEGER;

  this->SampleRate[0] = this->SampleRate[1] = this->SampleRate[2] = 1;
}

//-----------------------------------------------------------------------------
// Get ALL of the input.
void vtkExtractVOI::ComputeInputUpdateExtent(int inExt[6], 
                                             int *)
{
  // request all of the VOI
  int *wholeExtent;
  int i;
  
  wholeExtent = this->GetInput()->GetWholeExtent();
  memcpy(inExt, wholeExtent, 6*sizeof(int));

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
}

//-----------------------------------------------------------------------------
void 
vtkExtractVOI::ExecuteInformation(vtkImageData *input, vtkImageData *output)
{
  int i, dims[3], outDims[3], voi[6];
  int rate[3];
  int wholeExtent[6];
  
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("Missing input");
    return;
    }
  input->GetWholeExtent( wholeExtent );
  dims[0] = wholeExtent[1] - wholeExtent[0] + 1;
  dims[1] = wholeExtent[3] - wholeExtent[2] + 1;
  dims[2] = wholeExtent[5] - wholeExtent[4] + 1;
  
  
  for ( i=0; i < 6; i++ )
    {
    voi[i] = this->VOI[i];
    }

  for ( i=0; i < 3; i++ )
    {
    if ( voi[2*i+1] >= wholeExtent[2*i+1] )
      {
      voi[2*i+1] = wholeExtent[2*i+1];
      }
    else if ( voi[2*i+1] < wholeExtent[2*1] )
      {
      voi[2*i+1] = wholeExtent[2*i];
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
    }

  // This makes sense for sample rates of 1, 1, 1.
  wholeExtent[0] = voi[0];
  wholeExtent[1] = voi[0] + outDims[0] - 1;
  wholeExtent[2] = voi[2];
  wholeExtent[3] = voi[2] + outDims[1] - 1;
  wholeExtent[4] = voi[4];
  wholeExtent[5] = voi[4] + outDims[2] - 1;
  
  output->SetWholeExtent( wholeExtent );
}

//-----------------------------------------------------------------------------
void vtkExtractVOI::ExecuteData(vtkDataObject *)
{
  vtkImageData *input=this->GetInput();
  vtkPointData *pd=input->GetPointData();
  vtkCellData *cd=input->GetCellData();
  vtkImageData *output = this->GetOutput();
  output->SetExtent(output->GetWholeExtent());
  output->AllocateScalars();
  vtkPointData *outPD=output->GetPointData();
  vtkCellData *outCD=output->GetCellData();
  int i, j, k, dims[3], outDims[3], voi[6], dim, idx, newIdx;
  int newCellId;
  float origin[3], ar[3], outOrigin[3], outAR[3];
  int sliceSize, outSize, jOffset, kOffset, rate[3];
  int *wholeExtent = input->GetWholeExtent();
  int *inExt = input->GetExtent();
  int *outExt = output->GetExtent();

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

    outAR[i] = ar[i] * this->SampleRate[i];
    outOrigin[i] = origin[i] + voi[2*i]*ar[i] - outExt[2*i]*outAR[i];
    outSize *= outDims[i];
    }
  
  output->SetSpacing(outAR);
  output->SetOrigin(outOrigin);
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




