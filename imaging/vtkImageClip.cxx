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
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageClip* vtkImageClip::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageClip");
  if(ret)
    {
    return (vtkImageClip*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageClip;
}





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
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->OutputWholeExtent[idx] != extent[idx])
      {
      this->OutputWholeExtent[idx] = extent[idx];
      this->Modified();
      }
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
void vtkImageClip::Execute(vtkImageData *inData, 
			   vtkImageData *outData)
{
  int *inExt, *outExt;
  
  outExt = outData->GetUpdateExtent();  
  inExt  = inData->GetExtent(); 

  if (this->ClipData && 
      (inExt[0]<outExt[0] || inExt[1]>outExt[1] || 
       inExt[2]<outExt[2] || inExt[3]>outExt[3] ||
       inExt[4]<outExt[4] || inExt[5]>outExt[5]    ))
    {
    outData->SetExtent(outExt);
    outData->AllocateScalars();
    this->CopyData(inData, outData, outExt);
    outData->DataHasBeenGenerated();
    }
  else
    {
    outData->SetExtent(inExt);
    outData->GetPointData()->PassData(inData->GetPointData());
    outData->DataHasBeenGenerated();
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
  int idxY, idxZ, maxY, maxZ;
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


//----------------------------------------------------------------------------
void vtkImageClip::SetOutputWholeExtent(int piece, int numPieces)
{
  vtkImageData *input = this->GetInput();
  int ext[6];

  if (input == NULL)
    {
    vtkErrorMacro("We must have an input to set the output extent by piece.");
    return;
    }

  input->UpdateInformation();
  input->GetWholeExtent(ext);
  this->SplitExtentTmp(piece, numPieces, ext);

  this->SetOutputWholeExtent(ext);
}

//----------------------------------------------------------------------------
// Assumes UpdateInformation was called first.
int vtkImageClip::SplitExtentTmp(int piece, int numPieces, int *ext)
{
  int numPiecesInFirstHalf;
  int size[3], mid, splitAxis;

  // keep splitting until we have only one piece.
  // piece and numPieces will always be relative to the current ext. 
  while (numPieces > 1)
    {
    // Get the dimensions for each axis.
    size[0] = ext[1]-ext[0];
    size[1] = ext[3]-ext[2];
    size[2] = ext[5]-ext[4];
    // choose the biggest axis
    if (size[2] >= size[1] && size[2] >= size[0] && 
	size[2]/2 >= 2)
      {
      splitAxis = 2;
      }
    else if (size[1] >= size[0] && size[1]/2 >= 2)
      {
      splitAxis = 1;
      }
    else if (size[0]/2 >= 2)
      {
      splitAxis = 0;
      }
    else
      {
      // signal no more splits possible
      splitAxis = -1;
      }

    if (splitAxis == -1)
      {
      // can not split any more.
      if (piece == 0)
        {
        // just return the remaining piece
        numPieces = 1;
        }
      else
        {
        // the rest must be empty
        return 0;
        }
      }
    else
      {
      // split the chosen axis into two pieces.
      numPiecesInFirstHalf = (numPieces / 2);
      mid = (size[splitAxis] * numPiecesInFirstHalf / numPieces) 
	+ ext[splitAxis*2];
      if (piece < numPiecesInFirstHalf)
        {
        // piece is in the first half
        // set extent to the first half of the previous value.
        ext[splitAxis*2+1] = mid;
        // piece must adjust.
        numPieces = numPiecesInFirstHalf;
        }
      else
        {
        // piece is in the second half.
        // set the extent to be the second half. (two halves share points)
        ext[splitAxis*2] = mid;
        // piece must adjust
        numPieces = numPieces - numPiecesInFirstHalf;
        piece -= numPiecesInFirstHalf;
        }
      }
    } // end of while

  return 1;
}


