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
  int i, ext[6], voi[6];
  int *inWholeExt, *outWholeExt, *updateExt;
  

  inWholeExt = input->GetWholeExtent();
  outWholeExt = output->GetWholeExtent();
  updateExt = output->GetUpdateExtent();

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

  ext[0] = voi[0] + (updateExt[0]-outWholeExt[0])*this->SampleRate[0];
  ext[1] = voi[0] + (updateExt[1]-outWholeExt[0])*this->SampleRate[0];
  if (ext[1] > voi[1])
    { // This handles the IncludeBoundary condition.
    ext[1] = voi[1];
    }
  ext[2] = voi[2] + (updateExt[2]-outWholeExt[2])*this->SampleRate[1];
  ext[3] = voi[2] + (updateExt[3]-outWholeExt[2])*this->SampleRate[1];
  if (ext[3] > voi[3])
    { // This handles the IncludeBoundary condition.
    ext[3] = voi[3];
    }
  ext[4] = voi[4] + (updateExt[4]-outWholeExt[4])*this->SampleRate[2];
  ext[5] = voi[4] + (updateExt[5]-outWholeExt[4])*this->SampleRate[2];
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
  
  input->SetUpdateExtent(ext);
  
}



void vtkExtractGrid::ExecuteInformation()
{
  vtkStructuredGrid *input= this->GetInput();
  vtkStructuredGrid *output= this->GetOutput();
  int i, outDims[3], voi[6], wholeExtent[6];
  int mins[3];
  int rate[3];

  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("Missing input");
    return;
    }

  this->vtkStructuredGridToStructuredGridFilter::ExecuteInformation();

  input->GetWholeExtent(wholeExtent);

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
      output->SetWholeExtent(0,-1,0,-1,0,-1);
      return;
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
    mins[i] = (int)(floor((float)voi[2*i] / (float)rate[i]));
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
  int i, j, k, *uExt, voi[6];
  int *inExt, *outWholeExt;
  int iIn, jIn, kIn;
  int outSize, jOffset, kOffset, *rate;
  vtkIdType idx, newIdx, newCellId;
  vtkPoints *newPts, *inPts;
  int inInc1, inInc2;

  vtkDebugMacro(<< "Extracting Grid");

  inPts = input->GetPoints();

  outWholeExt = output->GetWholeExtent();
  uExt = output->GetUpdateExtent();
  rate = this->SampleRate;
  inExt = input->GetExtent();
  inInc1 = (inExt[1]-inExt[0]+1);
  inInc2 = inInc1*(inExt[3]-inExt[2]+1);

  // Clip the VOI by the input extent
  for (i = 0; i < 3; ++i)
    {
    voi[i*2] = this->VOI[2*i];
    if (voi[2*i] < inExt[2*i])
      {
      voi[2*i] = inExt[2*i];
      }
    voi[i*2+1] = this->VOI[2*i+1];
    if (voi[2*i+1] > inExt[2*i+1])
      {
      voi[2*i+1] = inExt[2*i+1];
      }
    }

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
    return;
    }

  // Allocate necessary objects
  //
  outSize = (uExt[1]-uExt[0]+1)*(uExt[3]-uExt[2]+1)*(uExt[5]-uExt[4]+1);
  newPts = (vtkPoints *) inPts->MakeObject(); 
  newPts->SetNumberOfPoints(outSize);
  outPD->CopyAllocate(pd,outSize,outSize);
  outCD->CopyAllocate(cd,outSize,outSize);

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


