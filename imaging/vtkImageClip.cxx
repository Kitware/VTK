/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageClip.cxx
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

#include "vtkImageClip.h"


//----------------------------------------------------------------------------
vtkImageClip::vtkImageClip()
{
  int idx;

  this->ClipData = 0;
  this->Initialized = 0;
  for (idx = 0; idx < 3; ++idx)
    {
    this->OutputWholeExtent[idx*2]  = -VTK_LARGE_INTEGER;
    this->OutputWholeExtent[idx*2+1] = VTK_LARGE_INTEGER;
    }
}


//----------------------------------------------------------------------------
void vtkImageClip::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "OutputWholeExtent: (" << this->OutputWholeExtent[0]
     << "," << this->OutputWholeExtent[1];
  for (idx = 1; idx < 3; ++idx)
    {
    os << indent << ", " << this->OutputWholeExtent[idx * 2]
       << "," << this->OutputWholeExtent[idx*2 + 1];
    }
  os << ")\n";
  if (this->ClipData)
    {
    os << indent << "ClipDataOn\n";
    }
  else
    {
    os << indent << "ClipDataOff\n";
    }
}
  
//----------------------------------------------------------------------------
void vtkImageClip::SetOutputWholeExtent(int extent[6])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->OutputWholeExtent[idx] != extent[idx])
      {
      this->OutputWholeExtent[idx] = extent[idx];
      this->Modified();
      }
    }
  if (modified)
    {
    this->Modified();
    }
  this->Initialized = 1;
}

//----------------------------------------------------------------------------
void vtkImageClip::SetOutputWholeExtent(int minX, int maxX, 
					     int minY, int maxY,
					     int minZ, int maxZ)
{
  int extent[6];
  
  extent[0] = minX;  extent[1] = maxX;
  extent[2] = minY;  extent[3] = maxY;
  extent[4] = minZ;  extent[5] = maxZ;
  this->SetOutputWholeExtent(extent);
}


//----------------------------------------------------------------------------
void vtkImageClip::GetOutputWholeExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->OutputWholeExtent[idx];
    }
}


//----------------------------------------------------------------------------
// Change the WholeExtent
void vtkImageClip::ExecuteInformation(vtkImageData *inData, 
				      vtkImageData *outData)
{
  int idx, extent[6];
  
  inData->GetWholeExtent(extent);
  if ( ! this->Initialized)
    {
    this->SetOutputWholeExtent(extent);
    }
  if ( ! this->Bypass)
    {
    // Clip the OutputWholeExtent with the input WholeExtent
    for (idx = 0; idx < 3; ++idx)
      {
      if (this->OutputWholeExtent[idx*2] >= extent[idx*2] && 
	  this->OutputWholeExtent[idx*2] <= extent[idx*2+1])
	{
	extent[idx*2] = this->OutputWholeExtent[idx*2];
	}
      if (this->OutputWholeExtent[idx*2+1] >= extent[idx*2] && 
	  this->OutputWholeExtent[idx*2+1] <= extent[idx*2+1])
	{
	extent[idx*2+1] = this->OutputWholeExtent[idx*2+1];
	}
      // make usre the order is correct
      if (extent[idx*2] > extent[idx*2+1])
	{
	extent[idx*2] = extent[idx*2+1];
	}
      }
    }
  
  outData->SetWholeExtent(extent);
}


//----------------------------------------------------------------------------
// Sets the output whole extent to be the input whole extent.
void vtkImageClip::ResetOutputWholeExtent()
{
  if ( ! this->GetInput())
    {
    vtkWarningMacro("ResetOutputWholeExtent: No input");
    return;
    }

  this->GetInput()->UpdateInformation();
  this->SetOutputWholeExtent(this->GetInput()->GetWholeExtent());
}



//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
void vtkImageClip::InternalUpdate(vtkDataObject *outObject)
{
  int *inExt, *outExt;
  vtkImageData *outData = (vtkImageData *)(outObject);
  vtkImageData *inData = this->GetInput();
  
  // Make sure the Input has been set.
  if ( ! inData)
    {
    vtkErrorMacro(<< "Input is not set.");
    return;
    }

  outExt = outData->GetUpdateExtent();
  inData->SetUpdateExtent(outExt);
  inData->Update();
  inExt = inData->GetExtent(); 

  if (this->ClipData && 
      (inExt[0]<outExt[0] || inExt[1]>outExt[1] || 
       inExt[2]<outExt[2] || inExt[3]>outExt[3] ||
       inExt[4]<outExt[4] || inExt[5]>outExt[5]    ))
    {
    outData->SetExtent(outExt);
    outData->AllocateScalars();
    this->CopyData(inData, outData, outExt);
    }
  else
    {
    outData->SetExtent(inExt);
    outData->GetPointData()->PassData(inData->GetPointData());
    }

  // release input data
  if (this->GetInput()->ShouldIReleaseData())
    {
    this->GetInput()->ReleaseData();
    }
}




//----------------------------------------------------------------------------
void vtkImageClip::CopyData(vtkImageData *inData, vtkImageData *outData,
                            int *ext)
{
  int idxX, idxY, idxZ, maxY, maxZ;
  int inIncX, inIncY, inIncZ, rowLength;
  unsigned char *inPtr, *inPtr1, *outPtr;
  
  
  inPtr = (unsigned char *) inData->GetScalarPointerForExtent(ext);
  outPtr = (unsigned char *) outData->GetScalarPointer();
  
  // Get increments to march through inData 
  inData->GetIncrements(inIncX, inIncY, inIncZ);
  
  // find the region to loop over
  rowLength = (ext[1] - ext[0]+1)*inIncX*inData->GetScalarSize();
  maxY = ext[3] - ext[2]; 
  maxZ = ext[5] - ext[4];
  
  inIncY *= inData->GetScalarSize(); 
  inIncZ *= inData->GetScalarSize();
  
  // Loop through outData pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    inPtr1 = inPtr + idxZ*inIncZ;
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      memcpy(outPtr,inPtr1,rowLength);
      inPtr1 += inIncY;
      outPtr += rowLength;
      }
    }
}
