/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataStreamer.cxx
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

#include "vtkImageDataStreamer.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkImageDataStreamer* vtkImageDataStreamer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageDataStreamer");
  if(ret)
    {
    return (vtkImageDataStreamer*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageDataStreamer;
}


//----------------------------------------------------------------------------
vtkImageDataStreamer::vtkImageDataStreamer()
{
  this->NumberOfDivisions = 1;
  this->SplitMode = VTK_IMAGE_DATA_STREAMER_SLAB_MODE;
}


//----------------------------------------------------------------------------
void vtkImageDataStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);
  os << indent << "NumberOfDivisions: " << this->NumberOfDivisions << endl;
  os << indent << "SplitMode: ";
  if (this->SplitMode == VTK_IMAGE_DATA_STREAMER_BLOCK_MODE)
    {
    os << "Block\n";
    }
  else if (this->SplitMode == VTK_IMAGE_DATA_STREAMER_SLAB_MODE)
    {
    os << "Block\n";
    }
  else
    {
    os << "Unknown\n";
    }
}
  
//----------------------------------------------------------------------------
void vtkImageDataStreamer::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageDataStreamer::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkImageDataStreamer::PreUpdate(vtkDataObject *vtkNotUsed(out))
{
  // Do nothing here (for now).
}

//----------------------------------------------------------------------------
void vtkImageDataStreamer::InternalUpdate(vtkDataObject *out)
{
  vtkImageData *input = this->GetInput();
  vtkImageData *output = (vtkImageData*)out;
  int ext[6];
  int idx;
  
  output->ReleaseData();
  
  // Try to behave gracefully with no input.
  if (input == NULL)
    {
    output->SetExtent(output->GetUpdateExtent());
    output->AllocateScalars();
    output->DataHasBeenGenerated();
    return;
    }
  
  // Fast path for when there is no streaming.
  if (this->NumberOfDivisions <= 1)
    {
    input->SetUpdateExtent(output->GetUpdateExtent());
    input->Update();
    output->SetExtent(input->GetExtent());
    output->GetPointData()->PassData(input->GetPointData());
    output->DataHasBeenGenerated();
    return;
    }
  
  // If we actually need to break up the input request.
  output->GetUpdateExtent(ext);
  output->SetExtent(ext);
  output->AllocateScalars();
  for (idx = 0; idx < this->NumberOfDivisions; ++idx)
    {
    vtkDebugMacro("Streaming piece " << idx << " of " 
		  << this->NumberOfDivisions << endl);
    output->GetUpdateExtent(ext);
    this->SplitExtent(ext, idx, this->NumberOfDivisions);
    input->SetUpdateExtent(ext);
    input->Update();
    output->CopyAndCastFrom(input, ext);
    }
  output->DataHasBeenGenerated();
}

//----------------------------------------------------------------------------
// Assumes UpdateInformation was called first.
int vtkImageDataStreamer::SplitExtent(int *ext, int piece, int numPieces)
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
    if (this->SplitMode == VTK_IMAGE_DATA_STREAMER_BLOCK_MODE)
      {
      // BLOCK MODE: choose the biggest axis
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
      }
    else
      {
      // SLAB MODE: split z down to one slice ...
      if (size[2] > 1)
	{
	splitAxis = 2;
	}
      else if (size[1] > 1)
	{
	splitAxis = 1;
	}
      else if (size[0] > 1)
	{
	splitAxis = 0;
	}
      else
	{
	splitAxis = -1;
	}
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
