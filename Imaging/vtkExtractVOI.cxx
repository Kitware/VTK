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

vtkCxxRevisionMacro(vtkExtractVOI, "1.27");
vtkStandardNewMacro(vtkExtractVOI);

// Construct object to extract all of the input data.
vtkExtractVOI::vtkExtractVOI()
{
  this->VOI[0] = this->VOI[2] = this->VOI[4] = 0;
  this->VOI[1] = this->VOI[3] = this->VOI[5] = VTK_LARGE_INTEGER;

  this->SampleRate[0] = this->SampleRate[1] = this->SampleRate[2] = 1;
}

void vtkExtractVOI::ExecuteInformation()
{
  vtkImageData *input=this->GetInput();
  vtkStructuredPoints *output=this->GetOutput();
  int i, dims[3], outDims[3], voi[6];
  int rate[3];
  int wholeExtent[6];
  
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("Missing input");
    return;
    }
  this->vtkStructuredPointsToStructuredPointsFilter::ExecuteInformation();

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
    if ( voi[2*i+1] >= dims[i] )
      {
      voi[2*i+1] = dims[i] - 1;
      }
    else if ( voi[2*i+1] < 0 )
      {
      voi[2*i+1] = 0;
      }

    if ( voi[2*i] > voi[2*i+1] )
      {
      voi[2*i] = voi[2*i+1];
      }
    else if ( voi[2*i] < 0 )
      {
      voi[2*i] = 0;
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

  wholeExtent[0] = 0;
  wholeExtent[1] = outDims[0] - 1;
  wholeExtent[2] = 0;
  wholeExtent[3] = outDims[1] - 1;
  wholeExtent[4] = 0;
  wholeExtent[5] = outDims[2] - 1;
  
  output->SetWholeExtent( wholeExtent );
  output->SetScalarType( input->GetScalarType() );
}

void vtkExtractVOI::Execute()
{
  vtkImageData *input=this->GetInput();
  vtkPointData *pd=input->GetPointData();
  vtkCellData *cd=input->GetCellData();
  vtkStructuredPoints *output=this->GetOutput();
  vtkPointData *outPD=output->GetPointData();
  vtkCellData *outCD=output->GetCellData();
  int i, j, k, dims[3], outDims[3], voi[6], dim, idx, newIdx;
  int newCellId;
  float origin[3], ar[3], outOrigin[3], outAR[3];
  int sliceSize, outSize, jOffset, kOffset, rate[3];

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
    if ( voi[2*i+1] >= dims[i] )
      {
      voi[2*i+1] = dims[i] - 1;
      }
    else if ( voi[2*i+1] < 0 )
      {
      voi[2*i+1] = 0;
      }

    if ( voi[2*i] > voi[2*i+1] )
      {
      voi[2*i] = voi[2*i+1];
      }
    else if ( voi[2*i] < 0 )
      {
      voi[2*i] = 0;
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
    outOrigin[i] = origin[i] + voi[2*i]*ar[i];
    outSize *= outDims[i];
    }

  output->SetDimensions(outDims);
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
    kOffset = k * sliceSize;
    for ( j=voi[2]; j <= voi[3]; j += rate[1] )
      {
      jOffset = j * dims[0];
      for ( i=voi[0]; i <= voi[1]; i += rate[0] )
        {
        idx = i + jOffset + kOffset;
        outPD->CopyData(pd, idx, newIdx++);
        }
      }
    }

//
// Traverse input data and copy cell attributes to output
//
  newCellId = 0;
  sliceSize = (dims[0]-1)*(dims[1]-1);
  for ( k=voi[4]; k < voi[5]; k += rate[2] )
    {
    kOffset = k * sliceSize;
    for ( j=voi[2]; j < voi[3]; j += rate[1] )
      {
      jOffset = j * (dims[0] - 1);
      for ( i=voi[0]; i < voi[1]; i += rate[0] )
        {
        idx = i + jOffset + kOffset;
        outCD->CopyData(cd, idx, newCellId++);
        }
      }
    }

  vtkDebugMacro(<<"Extracted " << newIdx << " point attributes on "
                << dim << "-D dataset\n\tDimensions are (" << outDims[0]
                << "," << outDims[1] << "," << outDims[2] <<")");
}


void vtkExtractVOI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VOI: \n";
  os << indent << "  Imin,Imax: (" << this->VOI[0] << ", " << this->VOI[1] << ")\n";
  os << indent << "  Jmin,Jmax: (" << this->VOI[2] << ", " << this->VOI[3] << ")\n";
  os << indent << "  Kmin,Kmax: (" << this->VOI[4] << ", " << this->VOI[5] << ")\n";

  os << indent << "Sample Rate: (" << this->SampleRate[0] << ", "
               << this->SampleRate[1] << ", "
               << this->SampleRate[2] << ")\n";
}


