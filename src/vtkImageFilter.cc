/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFilter.cc
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
#include "vtkImageFilter.hh"
#include "vtkImageCache.hh"


//----------------------------------------------------------------------------
// Description:
// Constructor.
vtkImageFilter::vtkImageFilter()
{
  this->Input = NULL;
}


//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline upto and including this filter
// It propagates the message back. 
// (Note: current implementation may create a cascade of MTime requests.)
unsigned long int vtkImageFilter::GetPipelineMTime()
{
  unsigned long int time1, time2;

  // This objects MTime
  // (Super class considers cache in case cache did not originate message)
  time1 = this->vtkImageCachedSource::GetPipelineMTime();
  if ( ! this->Input)
    {
    vtkWarningMacro(<< "GetPipelineMTime: Input not set.");
    return time1;
    }
  
  // Pipeline mtime 
  time2 = this->Input->GetPipelineMTime();
  
  // Return the larger of the two 
  if (time2 > time1)
    time1 = time2;

  return time1;
}


//----------------------------------------------------------------------------
// Description:
// Set the Input of a filter. (A virtual method)
void vtkImageFilter::SetInput(vtkImageSource *input)
{
  vtkDebugMacro(<< "SetInput: input = " << input->GetClassName()
		<< " (" << input << ")");

  this->Input = input;
  this->Modified();
}


//----------------------------------------------------------------------------
// Description:
// This method gets the input tile necessary to generate the Region,
// gets the output tile from the cache, and executes the filter.
// If the input tile request fails, the region of interest is broken 
// into smaller more manigable pieces.
// Any subclass that over rides this default functions must handle
// input tile request failure itself.
// Getting the cache ouput tile is guarenteed to succeed.
void vtkImageFilter::GenerateRegion(int *outOffset, int *outSize)
{
  int inOffset[3], inSize[3];
  vtkImageRegion *inRegion;
  vtkImageRegion *outRegion;
  
  vtkDebugMacro(<< "GenerateRegion: offset = (" 
                << outOffset[0] << ", " << outOffset[1] << ", " << outOffset[2]
                << "), size = (" 
                << outSize[0] << ", " << outSize[1] << ", " << outSize[2] 
                << ")");

  // make sure the Input has been set 
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Input is not set.");
    return;
    }
  
  // get the required input Region 
  this->RequiredRegion(outOffset, outSize, inOffset, inSize);

  // get the required tile from the Input 
  inRegion = this->Input->RequestRegion(inOffset, inSize);

  // Make sure the requested tile was not too large 
  if ( ! inRegion)
    {
    // Call alternative slower generate that breaks the task into pieces 
    this->GenerateRegionTiled(outOffset, outSize);
    return;
    }
  
  // get the output tile from the cache
  if ( ! this->Cache)
    {
    vtkErrorMacro(<< "GenerateRegion: Filter has no cache object.");
    inRegion->Delete();
    return;
    }
  outRegion = this->Cache->GetRegion(outOffset, outSize);

  // fill the output tile 
  this->Execute(inRegion, outRegion);

  // free the input tile 
  inRegion->Delete();
}

//----------------------------------------------------------------------------
// Description:
// This method returns in "offset" and "size" the boundary of data
// in the image. Request for regions of the image out side of these
// bounds will have unpridictable effects.
// i.e. no error checking is performed.
void vtkImageFilter::GetBoundary(int *offset, int *size)
{
  vtkDebugMacro(<< "GetBoundary:");
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "GetBoundary: No input");
    return;
    }
  this->Input->GetBoundary(offset, size);

  vtkDebugMacro(<< "GetBoundary: Returning offset = ..., size = ...");
  
  return;
}

//----------------------------------------------------------------------------
// Description:
// This method computes the Region of the input necessary to 
// generate outRegion.
void vtkImageFilter::RequiredRegion(int *outOffset, int *outSize,
				int *inOffset, int *inSize)
{
  // Here to avoid warnings
  outOffset = outOffset;
  outSize = outSize;
  inOffset = inOffset;
  inSize = inSize;

  vtkErrorMacro(<< "RequiredRegion method is not specified for this filter.");
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output tile, and executes the filter
// algorithm to fill the output from the input.
void vtkImageFilter::Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  // Here to avoid warnings
  inRegion = inRegion;
  outRegion = outRegion;

  vtkErrorMacro(<< "Execute method is not specified for this filter.");
}



/*****************************************************************************
  Stuff for executing the filter in pieces.
*****************************************************************************/

//----------------------------------------------------------------------------
// Description:
// This method generates the out Region in pieces.
void vtkImageFilter::GenerateRegionTiled(int *outOffset, int *outSize)
{
  int genericPieceSize[3];     
  int pieceOffset[3], pieceSize[3];
  int deltaOffset[3];

  vtkDebugMacro(<< "GenerateRegionTiled: outRegion must be split into pieces");

  // Have we bottomed out splitting the tile and gotten to one pixel?
  if (outSize[0] <= 1 && outSize[1] <= 1 && outSize[2] <= 1)
    {
    vtkErrorMacro(<< "Cannot split any more. (outRegion is only one pixel)");
    return;
    }

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
void vtkImageFilter::SplitRegion(int *outOffset, int *outSize, int *pieceSize)
{
  int newSize[3];
  int inSize[3], inOffset[3];
  int memory;
  int bestMemory;

  // split down the first axis (two pieces, round down) 
  newSize[1] = outSize[1];
  newSize[2] = outSize[2];
  if (1 < outSize[0] && outSize[0] < 5 )    // avoid remainder slivers
    newSize[0] = outSize[0] / 2;  
  else
    newSize[0] = 1 + outSize[0] / 2;  
  // determine the input Region for this newSize 
  this->RequiredRegion(outOffset, newSize, inOffset, inSize);
  // calculate the memory needed 
  memory = inSize[0] * inSize[1] * inSize[2];
  // save the best (smallest memory) so far 
  bestMemory = memory;
  pieceSize[0]=newSize[0];  pieceSize[1]=newSize[1];  pieceSize[2]=newSize[2];

  // split down the second axis (two pieces, round down) 
  newSize[0] = outSize[0];
  newSize[2] = outSize[2];
  if (1 < outSize[1] && outSize[1] < 5 )    // avoid remainder slivers
    newSize[1] = outSize[1] / 2;  
  else
    newSize[1] = 1 + outSize[1] / 2;  
  // determine the input Region for this newSize 
  this->RequiredRegion(outOffset, newSize, inOffset, inSize);
  // calculate the memory needed 
  memory = inSize[0] * inSize[1] * inSize[2];
  // save the best (smallest memory) so far 
  if (memory < bestMemory)
    {
    bestMemory = memory;
    pieceSize[0]=newSize[0]; pieceSize[1]=newSize[1]; pieceSize[2]=newSize[2];
    }

  // split down the first axis (two pieces, round up) 
  newSize[0] = outSize[0];
  newSize[1] = outSize[1];
  if (1 < outSize[2] && outSize[2] < 5 )    // avoid remainder slivers
    newSize[2] = outSize[2] / 2;  
  else
    newSize[2] = 1 + outSize[2] / 2;  
  // determine the input Region for this newSize 
  this->RequiredRegion(outOffset, newSize, inOffset, inSize);
  // calculate the memory needed 
  memory = inSize[0] * inSize[1] * inSize[2];
  // save the best (smallest memory) so far 
  if (memory < bestMemory)
    {
    bestMemory = memory;
    pieceSize[0]=newSize[0]; pieceSize[1]=newSize[1]; pieceSize[2]=newSize[2];
    }
}
















