/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractGrid.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkExtractGrid.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------
vtkExtractGrid* vtkExtractGrid::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkExtractGrid");
  if(ret)
    {
    return (vtkExtractGrid*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkExtractGrid;
}

// Construct object to extract all of the input data.
vtkExtractGrid::vtkExtractGrid()
{
  this->VOI[0] = this->VOI[2] = this->VOI[4] = 0;
  this->VOI[1] = this->VOI[3] = this->VOI[5] = VTK_LARGE_INTEGER;

  this->SampleRate[0] = this->SampleRate[1] = this->SampleRate[2] = 1;
  
  this->IncludeBoundary = 0;
}


void vtkExtractGrid::ComputeInputUpdateExtents(vtkDataObject *vtkNotUsed(out))
{
  vtkStructuredGrid *input = this->GetInput();
  vtkStructuredGrid *output = this->GetOutput();
  int ext[6];
  int *wholeExt;
  
  output->GetUpdateExtent(ext);
  ext[0] = ext[0] * this->SampleRate[0];
  ext[1] = ext[1] * this->SampleRate[0];
  ext[2] = ext[2] * this->SampleRate[1];
  ext[3] = ext[3] * this->SampleRate[1];
  ext[4] = ext[4] * this->SampleRate[2];
  ext[5] = ext[5] * this->SampleRate[2];
  
  
  wholeExt = input->GetWholeExtent();
  
  if (ext[0] < wholeExt[0])
    {
    ext[0] = wholeExt[0];
    }
  if (ext[1] > wholeExt[1])
    {
    ext[1] = wholeExt[1];
    }
  
  if (ext[2] < wholeExt[2])
    {
    ext[2] = wholeExt[2];
    }
  if (ext[3] > wholeExt[3])
    {
    ext[3] = wholeExt[3];
    }
  
  if (ext[4] < wholeExt[4])
    {
    ext[4] = wholeExt[4];
    }
  if (ext[5] > wholeExt[5])
    {
    ext[5] = wholeExt[5];
    }  
  
  input->SetUpdateExtent(ext);
  
}



void vtkExtractGrid::ExecuteInformation()
{
  vtkStructuredGrid *input= this->GetInput();
  vtkStructuredGrid *output= this->GetOutput();
  int i, dims[3], outDims[3], voi[6], wholeExtent[6];
  int rate[3];

  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("Missing input");
    return;
    }

  this->vtkStructuredGridToStructuredGridFilter::ExecuteInformation();

  input->GetWholeExtent(wholeExtent);
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
  wholeExtent[0] = 0;
  wholeExtent[1] = outDims[0] - 1;
  wholeExtent[2] = 0;
  wholeExtent[3] = outDims[1] - 1;
  wholeExtent[4] = 0;
  wholeExtent[5] = outDims[2] - 1;

  output->SetWholeExtent(wholeExtent);
}

void vtkExtractGrid::Execute()
{
  vtkStructuredGrid *input= this->GetInput();
  vtkPointData *pd=input->GetPointData();
  vtkCellData *cd=input->GetCellData();
  vtkStructuredGrid *output= this->GetOutput();
  vtkPointData *outPD=output->GetPointData();
  vtkCellData *outCD=output->GetCellData();
  int i, j, k, dims[3], outDims[3], voi[6], dim;
  vtkIdType idx, newIdx, newCellId;
  int sliceSize, outSize, jOffset, kOffset, rate[3];
  vtkPoints *newPts, *inPts;
  int includeBoundary[3], diff;

  vtkDebugMacro(<< "Extracting Grid");

  inPts = input->GetPoints();
  input->GetDimensions(dims);

  // Get the volume of interest.
  // Make sure parameters are consistent.
  for ( i=0; i < 6; i++ )
    {
    voi[i] = this->VOI[i];
    }

  includeBoundary[0] = includeBoundary[1] = includeBoundary[2] = 0;
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

    if ( this->IncludeBoundary && rate[i] != 1 )
      {
      if ( ((diff=voi[2*i+1]-voi[2*i]) > 0) && ((diff % rate[i]) != 0) )
        {
        outDims[i]++;
        includeBoundary[i] = 1;
        }
      }
    
    outSize *= outDims[i];
    }

  output->SetDimensions(outDims);

  // If output same as input, just pass data through
  //
  if ( outDims[0] == dims[0] && outDims[1] == dims[1] && outDims[2] == dims[2] &&
  rate[0] == 1 && rate[1] == 1 && rate[2] == 1 )
    {
    output->SetPoints(inPts);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    vtkDebugMacro(<<"Passed data through bacause input and output are the same");
    return;
    }

  // Allocate necessary objects
  //
  newPts = (vtkPoints *) inPts->MakeObject(); 
  newPts->SetNumberOfPoints(outSize);
  outPD->CopyAllocate(pd,outSize,outSize);
  outCD->CopyAllocate(cd,outSize,outSize);

  // Traverse input data and copy point attributes to output
  //
  sliceSize = dims[0]*dims[1];
  newIdx = 0;
  for ( k=voi[4]; k <= voi[5]; )
    {
    kOffset = k * sliceSize;
    for ( j=voi[2]; j <= voi[3]; )
      {
      jOffset = j * dims[0];
      for ( i=voi[0]; i <= voi[1]; )
        {
        idx = i + jOffset + kOffset;
        newPts->SetPoint(newIdx,inPts->GetPoint(idx));
        outPD->CopyData(pd, idx, newIdx++);

        i += rate[0]; //adjust for boundary
        if ( includeBoundary[0] && i > voi[1] && (i-rate[0]) != voi[1] )
          {
          i = voi[1];
          }
        }
      j += rate[1]; //adjust for boundary
      if ( includeBoundary[1] && j > voi[3] && (j-rate[1]) != voi[3] )
        {
        j = voi[3];
        }
      }
    k += rate[2]; //adjust for boundary
    if ( includeBoundary[2] && k > voi[5] && (k-rate[2]) != voi[5] )
      {
      k = voi[5];
      }
    }

  // Traverse input data and copy cell attributes to output
  //
  newCellId = 0;
  sliceSize = (dims[0]-1)*(dims[1]-1);
  for ( k=voi[4]; k < voi[5]; )
    {
    kOffset = k * sliceSize;
    for ( j=voi[2]; j < voi[3]; )
      {
      jOffset = j * (dims[0] - 1);
      for ( i=voi[0]; i < voi[1]; )
        {
        idx = i + jOffset + kOffset;
        outCD->CopyData(cd, idx, newCellId++);

        i += rate[0]; //adjust for boundary
        if ( includeBoundary[0] && i >= voi[1] && (i-rate[0]) != (voi[1]-1) )
          {
          i = voi[1] - 1;
          }
        }
      j += rate[1]; //adjust for boundary
      if ( includeBoundary[1] && j >= voi[3] && (j-rate[1]) != (voi[3]-1) )
        {
        j = voi[3] - 1;
        }
      }
    k += rate[2]; //adjust for boundary
    if ( includeBoundary[2] && k >= voi[5] && (k-rate[2]) != (voi[5]-1) )
      {
      k = voi[5] - 1;
      }
    }

  vtkDebugMacro(<<"Extracted " << newIdx << " point attributes on "
                << dim << "-D dataset\n\tDimensions are (" << outDims[0]
                << "," << outDims[1] << "," << outDims[2] <<")");

  output->SetPoints(newPts);
  newPts->Delete();
}


void vtkExtractGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredGridToStructuredGridFilter::PrintSelf(os,indent);

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


