/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDyadicFilter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "vtkImageDyadicFilter.hh"
#include "vtkImageCache.hh"
#include "vtkImageRegion.hh"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageDyadicFilter fitler.
vtkImageDyadicFilter::vtkImageDyadicFilter()
{
  this->Input1 = NULL;
  this->Input2 = NULL;

  this->Input1Data = NULL;
}


//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline upto and including this filter
// It propagates the message back. 
// (Note: current implementation may create a cascade of MTime requests.)
unsigned long int vtkImageDyadicFilter::GetPipelineMTime()
{
  unsigned long int time, temp;

  // This objects MTime
  // (Super class considers cache in case cache did not originate message)
  time = this->vtkImageCachedSource::GetPipelineMTime();

  // Input1 MTime
  if ( ! this->Input1)
    {
    vtkWarningMacro(<< "GetPipelineMTime: Input1 not set.");
    }
  else
    {
    temp = this->Input1->GetPipelineMTime();
    // Save the larger of the two 
    if (temp > time)
      time = temp;
    }

  // Input1 MTime
  if ( ! this->Input2)
    {
    vtkWarningMacro(<< "GetPipelineMTime: Input2 not set.");
    }
  else
    {
    temp = this->Input2->GetPipelineMTime();
    // Save the larger of the two 
    if (temp > time)
      time = temp;
    }

  return time;
}



//----------------------------------------------------------------------------
// Description:
// Set the first Input of a filter. (A virtual method)
void vtkImageDyadicFilter::SetInput1(vtkImageSource *input)
{
  vtkDebugMacro(<< "SetInput1: input = " << input->GetClassName()
		<< " (" << input << ")");

  this->Input1 = input;
  this->Modified();
}


//----------------------------------------------------------------------------
// Description:
// Set the Second Input of a filter. (A virtual method)
void vtkImageDyadicFilter::SetInput2(vtkImageSource *input)
{
  vtkDebugMacro(<< "SetInput2: input = " << input->GetClassName()
		<< " (" << input << ")");

  this->Input2 = input;
  this->Modified();
}


//----------------------------------------------------------------------------
// Description:
// This method gets the input tiles necessary to generate the Region,
// gets the output tile from the cache, and executes the filter.
// If an input tile request fails, the region of interest is broken into 
// smaller more manigable pieces.  First it splits for input 1 then input2.
// Any subclass that over rides this default functions must handle
// input-tile-request failures itself.
// Getting the cache ouput tile is guarenteed to succeed.
// Not set up to do splitting yet.
void vtkImageDyadicFilter::GenerateRegion(int *outOffset, int *outSize)
{
  int in1Offset[3], in1Size[3];
  int in2Offset[3], in2Size[3];
  vtkImageRegion *in1Region, *in2Region;
  vtkImageRegion *outRegion;
  
  vtkDebugMacro(<< "GenerateRegion: offset = (" 
                << outOffset[0] << ", " << outOffset[1] << ", " << outOffset[2]
                << "), size = (" 
                << outSize[0] << ", " << outSize[1] << ", " << outSize[2] 
                << ")");

  // make sure the Input has been set 
  if ( ! this->Input1 || ! this->Input2)
    {
    vtkErrorMacro(<< "An Input is not set.");
    return;
    }
  
  // get the required input Region 
  this->RequiredInput1Region(outOffset, outSize, in1Offset, in1Size);
  // get the required tiles from the First Input 
  in1Region = this->Input1->RequestRegion(in1Offset, in1Size);

  // get the required input Region 
  this->RequiredInput2Region(outOffset, outSize, in2Offset, in2Size);
  // get the required tiles from the Second Input 
  in2Region = this->Input2->RequestRegion(in2Offset, in2Size);

  // did we get the input?
  if ( ! in1Region || ! in2Region)
    {
    vtkErrorMacro(<< "Could not get input, and splitting does not work yet");
    return;
    }

  // get the output tile from the cache
  if ( ! this->Cache)
    {
    vtkErrorMacro(<< "GenerateRegion: Filter has no cache object.");
    in1Region->Delete();    
    in2Region->Delete();    
    return;
    }
  outRegion = this->Cache->GetRegion(outOffset, outSize);

  // fill the output tile 
  this->Execute(in1Region, in2Region, outRegion);

  // free the input tile 
  in1Region->Delete();
  in2Region->Delete();
}


//----------------------------------------------------------------------------
// Description:
// This method returns in "offset" and "size" the boundary of data
// in the image. Request for regions of the image out side of these
// bounds will have unpredictable effects.
// i.e. no error checking is performed.
void vtkImageDyadicFilter::GetBoundary(int *offset, int *size)
{
  int idx;
  int offset2[3], size2[3];
  int right1, right2;
  
  
  if ( ! this->Input1)
    {
    vtkErrorMacro(<< "GetBoundary: No input1");
    return;
    }
  if ( ! this->Input2)
    {
    vtkErrorMacro(<< "GetBoundary: No input2");
    return;
    }
  // Get the boundaries of the inputs
  this->Input1->GetBoundary(offset, size);
  this->Input2->GetBoundary(offset2, size2);
  // Take the intersection of these two boundaries
  for (idx = 0; idx < 3; ++idx)
    {
    right1 = offset[idx] + size[idx];
    right2 = offset2[idx] + size2[idx];
    offset[idx] = (offset2[idx] > offset[idx]) ? offset2[idx] : offset[idx];
    size[idx] = 
      (right2 > right1) ? (right1 - offset[idx]) : (right2 - offset[idx]);
    }
  
  vtkDebugMacro(<< "GetBoundary: returning offset = ("
        << offset[0] << ", " << offset[1] << ", " << offset[2]
        << "), size = (" << size[0] << ", " << size[1] << ", " << size[2]
        << ")");
    
  return;
}





//----------------------------------------------------------------------------
// Description:
// This method computes the Region from input1 necessary to generate outRegion.
// It is used by the default GenerateRegion method to create the in1Region.
// Boundaries are not considered yet.
void vtkImageDyadicFilter::RequiredInput1Region(int *outOffset, int *outSize,
					    int *in1Offset, int *in1Size)
{
  // Here to avoid warnings
  outOffset = outOffset;
  outSize = outSize;
  in1Offset = in1Offset;
  in1Size = in1Size;

  vtkErrorMacro(<< "RequiredInput1Region method is not specified.");
}



//----------------------------------------------------------------------------
// Description:
// This method computes the Region from input2 necessary to generate outRegion.
// It is used by the default GenerateRegion method to create the in2Region.
// Boundaries are not considered yet.
void vtkImageDyadicFilter::RequiredInput2Region(int *outOffset, int *outSize,
					    int *in2Offset, int *in2Size)
{
  // Here to avoid warnings
  outOffset = outOffset;
  outSize = outSize;
  in2Offset = in2Offset;
  in2Size = in2Size;

  vtkErrorMacro(<< "RequiredInput1Region method is not specified.");
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output tile, and executes the filter
// algorithm to fill the output from the input.
void vtkImageDyadicFilter::Execute(vtkImageRegion *in1Region, vtkImageRegion *in2Region, 
				  vtkImageRegion *outRegion)
{
  // Here to avoid warnings
  in1Region = in1Region;
  in2Region = in2Region;
  outRegion = outRegion;

  vtkErrorMacro(<< "Execute method is not specified for this filter.");
}



/*****************************************************************************
  Stuff for executing the filter in pieces.
*****************************************************************************/


//----------------------------------------------------------------------------
// Description:
// This method gets a tile from input1 in the Region.  If the data has already 
// been given to us (and stored in Input1Data), no request is made.
vtkImageRegion *vtkImageDyadicFilter::GetInput1Region(int *in1Offset, int *in1Size)
{
}

//----------------------------------------------------------------------------
// Description:
// This method gets a tile from input1 in the Region.  If the data has already 
// been given to us (and stored in Input1Data), no request is made.
vtkImageRegion *vtkImageDyadicFilter::GetInput2Region(int *in1Offset, int *in1Size)
{
}


//----------------------------------------------------------------------------
// Description:
// This design will not work, we have to come up with a different one.
void vtkImageDyadicFilter::ClearInputs()
{
}




//----------------------------------------------------------------------------
// Description:
// This method generates the out Region in pieces.
void vtkImageDyadicFilter::GenerateRegionTiled(int *outOffset, int *outSize)
{
  int genericPieceSize[3];     
  int pieceOffset[3], pieceSize[3];
  int deltaOffset[3];

  vtkDebugMacro(<< "GenerateRegionTiled: outRegion must be split into pieces");

  // choose a piece of the outRegion to determine how the tile is divided 
  // pieces near the edge of the tile can be smaller than this generic piece 
  this->SplitRegion(outOffset, outSize, genericPieceSize);

  // loop over the output Region generating the pieces 
  // start at the offset 
  deltaOffset[2] = 0;
  while (deltaOffset[2] < outSize[2])
    {
    pieceOffset[2] = outOffset[2] + deltaOffset[2];
    // make the piece smaller if it extends over edge 
    pieceSize[2] = outSize[2] - deltaOffset[2];
    if (pieceSize[2] > genericPieceSize[2])
      pieceSize[2] = genericPieceSize[2];

    deltaOffset[1] = 0;
    while (deltaOffset[1] < outSize[1])
      {
      pieceOffset[1] = outOffset[1] + deltaOffset[1];
      // make the piece smaller if it extends over edge 
      pieceSize[1] = outSize[1] - deltaOffset[1];
      if (pieceSize[1] > genericPieceSize[1])
	pieceSize[1] = genericPieceSize[1];

      deltaOffset[0] = 0;
      while (deltaOffset[0] < outSize[0])
	{
	pieceOffset[0] = outOffset[0] + deltaOffset[0];
	// make the piece smaller if it extends over edge 
	pieceSize[0] = outSize[0] - deltaOffset[0];
	if (pieceSize[0] > genericPieceSize[0])
	  pieceSize[0] = genericPieceSize[0];
	
	// Generate the data for this piece
	this->GenerateRegion(pieceOffset, pieceSize);
	
	deltaOffset[0] += genericPieceSize[0];
	}
      deltaOffset[1] += genericPieceSize[1];
      }
    deltaOffset[2] += genericPieceSize[2];
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is called when the output Region is too large to generate.
// It specifies how to split the Region into pieces by returning a generic
// pieceSize.  Over ride this method if you have a specific way of breaking 
// up a tile that is more efficient than this default.
void vtkImageDyadicFilter::SplitRegion(int *outOffset, int *outSize, 
				    int *pieceSize)
{
/*
  int newSize[3];
  int in1Size[3], in1Offset[3];
  int in2Size[3], in2Offset[3];
  int memory;
  int bestMemory;

  // split down the first axis (two pieces, round down) 
  newSize[1] = outSize[1];
  newSize[2] = outSize[2];
  newSize[0] = 1 + outSize[0] / 2;
  // determine the input Region for this newSize 
  this->RequiredRegion(outOffset, newSize, 
		    in1Offset, in1Size, in2Offset, in2Size);
  // calculate the memory needed (For the input we could not get)
  if (this->GetSplitingInput() == 1)
    memory = in1Size[0] * in1Size[1] * in1Size[2];
  else
    memory = in2Size[0] * in2Size[1] * in2Size[2];    
  // save the best (smallest memory) so far 
  bestMemory = memory;
  pieceSize[0]=newSize[0];  pieceSize[1]=newSize[1];  pieceSize[2]=newSize[2];

  // split down the second axis (two pieces, round down) 
  newSize[0] = outSize[0];
  newSize[2] = outSize[2];
  newSize[1] = 1 + outSize[1] / 2;
  // determine the input Region for this newSize 
  this->RequiredRegion(outOffset, newSize, 
		    in1Offset, in1Size, in2Offset, in2Size);
  // calculate the memory needed (For the input we could not get)
  if (this->GetSplittingInput() == 1)
    memory = in1Size[0] * in1Size[1] * in1Size[2];
  else
    memory = in2Size[0] * in2Size[1] * in2Size[2];    
  // save the best (smallest memory) so far 
  if (memory < bestMemory)
    {
    bestMemory = memory;
    pieceSize[0]=newSize[0]; pieceSize[1]=newSize[1]; pieceSize[2]=newSize[2];
    }

  // split down the first axis (two pieces, round up) 
  newSize[0] = outSize[0];
  newSize[1] = outSize[1];
  newSize[2] = 1 + outSize[2] / 2;
  // determine the input Region for this newSize 
  this->RequiredRegion(outOffset, newSize, 
		    in1Offset, in1Size, in2Offset, in2Size);
  // calculate the memory needed (For the input we could not get)
  if (this->GetSplittingInput() == 1)
    memory = in1Size[0] * in1Size[1] * in1Size[2];
  else
    memory = in2Size[0] * in2Size[1] * in2Size[2];    
  // save the best (smallest memory) so far 
  if (memory < bestMemory)
    {
    bestMemory = memory;
    pieceSize[0]=newSize[0]; pieceSize[1]=newSize[1]; pieceSize[2]=newSize[2];
    }
    */
}
















