/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageInPlaceFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkImageCache.h"
#include "vtkImageInPlaceFilter.h"

  
//----------------------------------------------------------------------------
// Description:
// This method is called by the cache.  It eventually calls the
// Execute(vtkImageData *, vtkImageData *) method.
// ImageInformation has already been updated by this point, 
// and outRegion is in local coordinates.
// This method will stream to get the input, and loops over extra axes.
// Only the UpdateExtent from output will get updated.
void vtkImageInPlaceFilter::InternalUpdate(vtkImageData *outData)
{
  int *inExt, *outExt;

  // Make sure the Input has been set.
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Input is not set.");
    return;
    }

  // prevent infinite update loops.
  if (this->Updating)
    {
    return;
    }
  this->Updating = 1;
  this->AbortExecute = 0;
  
  // Make sure there is an output.
  this->CheckCache();

  // In case this update is called directly.
  this->UpdateImageInformation();
  this->Output->ClipUpdateExtentWithWholeExtent();

  // Handle bypass condition.
  if (this->Bypass)
    {
    vtkImageData *inData;

    this->Input->SetUpdateExtent(this->Output->GetUpdateExtent());
    inData = this->Input->UpdateAndReturnData();
    if (!inData)
      {
      vtkWarningMacro("No input data provided!");
      }
    else
      {
      outData->GetPointData()->PassData(inData->GetPointData());
      }

    // release input data
    if (this->Input->ShouldIReleaseData())
      {
      this->Input->ReleaseData();
      }
    this->Updating = 0;
    return;
    }
  
  // since this is an in place filter the input and output extents 
  // better be the same. And the Release Flag better be set to
  // release ELSE we do the normal out of place filtering
  inExt = this->Input->GetUpdateExtent();
  outExt = this->Output->GetUpdateExtent();
  if ((inExt[0] == outExt[0])&&(inExt[1] == outExt[1])&&
      (inExt[2] == outExt[2])&&(inExt[3] == outExt[3])&&
      (inExt[4] == outExt[4])&&(inExt[5] == outExt[5])&&
      this->Input->ShouldIReleaseData())
    {
    vtkImageData *inData;
    inData = this->Input->UpdateAndReturnData();

    // pass the data
    outData->GetPointData()->PassData(inData->GetPointData());
    
    // The StartMethod call is placed here to be after updating the input.
    if ( this->StartMethod )
      {
      (*this->StartMethod)(this->StartMethodArg);
      }
    // fill the output region 
    this->Execute(inData, outData);
    if ( this->EndMethod )
      {
      (*this->EndMethod)(this->EndMethodArg);
      }
    
    // Like the graphics pipeline this source releases inputs data.
    this->Input->ReleaseData();
    }
  else
    {
    this->RecursiveStreamUpdate(outData,2);
    }
  
  this->Updating = 0;
}

//----------------------------------------------------------------------------
// Description:
// This method can be called recursively for streaming.
// The extent of the outRegion changes, dim remains the same.
// Same as the one in Filter except that it copies the data
// before executing
void vtkImageInPlaceFilter::RecursiveStreamUpdate(vtkImageData *outData,
						  int splitAxis)
{
  int memory;
  vtkImageData *inData;
  
  // Compute the required input region extent.
  // Copy to fill in extent of extra dimensions.
  this->ComputeRequiredInputUpdateExtent(this->Input->GetUpdateExtent(),
					 this->Output->GetUpdateExtent());
  
  // determine the amount of memory that will be used by the input region.
  memory = this->Input->GetUpdateExtentMemorySize();
  
  // Split the inRegion if we are streaming.
  if ((memory > this->Input->GetMemoryLimit()))
    {
    int min, max, mid;
    this->Output->GetAxisUpdateExtent(splitAxis,min,max);
    while ( (min == max) && splitAxis > 0)
      {
      splitAxis--;
      this->Output->GetAxisUpdateExtent(splitAxis,min,max);
      }
    // Make sure we can actually split the axis
    if (min < max)
      {
      // Set the first half to update
      mid = (min + max) / 2;
      vtkDebugMacro(<< "RecursiveStreamUpdate: Splitting " 
      << splitAxis << " : memory = " << memory <<
      ", extent = " << min << "->" << mid << " | " << mid+1 << "->" << max);
      this->Output->SetAxisUpdateExtent(splitAxis, min, mid);
      this->RecursiveStreamUpdate(outData, splitAxis);
      // Set the second half to update
      this->Output->SetAxisUpdateExtent(splitAxis, mid+1, max);
      this->RecursiveStreamUpdate(outData, splitAxis);
      // Restore the original extent
      this->Output->SetAxisUpdateExtent(splitAxis, min, max);
      return;
      }
    else
      {
      // Cannot split any more.  Ignore memory limit and continue.
      vtkWarningMacro(<< "RecursiveStreamUpdate: Cannot split. memory = "
        << memory << ", " << splitAxis << " : " << min << "->" << max);
      }
    }

  // No Streaming required.
  // Get the input region (Update extent was set at start of this method).
  inData = this->Input->UpdateAndReturnData();

  // The StartMethod call is placed here to be after updating the input.
  if ( this->StartMethod )
    {
    (*this->StartMethod)(this->StartMethodArg);
    }
  // fill the output region 
  // copy the data first
  this->CopyData(inData,outData);
  this->Execute(inData, outData);
  if ( this->EndMethod )
    {
    (*this->EndMethod)(this->EndMethodArg);
    }
  
  // Like the graphics pipeline this source releases inputs data.
  if (this->Input->ShouldIReleaseData())
    {
    this->Input->ReleaseData();
    }
}

void vtkImageInPlaceFilter::CopyData(vtkImageData *inData,
				     vtkImageData *outData)
{
  int *outExt = this->Output->GetUpdateExtent();
  char *inPtr = (char *) inData->GetScalarPointerForExtent(outExt);
  char *outPtr = (char *) outData->GetScalarPointerForExtent(outExt);
  int rowLength, size;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int idxY, idxZ, maxY, maxZ;
  
  rowLength = (outExt[1] - outExt[0]+1)*inData->GetNumberOfScalarComponents();
  size = inData->GetScalarSize();
  rowLength *= size;
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  
  // adjust increments for this loop
  inIncY = inIncY*size + rowLength;
  outIncY = outIncY*size + rowLength;
  inIncZ *= size;
  outIncZ *= size;
  
  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      memcpy(outPtr,inPtr,rowLength);
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}
