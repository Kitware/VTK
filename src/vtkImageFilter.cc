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
vtkImageFilter::vtkImageFilter()
{
  this->Input = NULL;
  this->UseExecuteMethodOn();
}

//----------------------------------------------------------------------------
void vtkImageFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageCachedSource::PrintSelf(os,indent);
  os << indent << "Input: (" << this->Input << ").\n";
  if (this->UseExecuteMethod)
    {
    os << indent << "Use Execute Method.\n";
    }
  else
    {
    os << indent << "Use Update Method.\n";
    }
}

//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline upto and including this filter
// Note: current implementation may create a cascade of GetPipelineMTime calls.
// Each GetPipelineMTime call propagates the call all the way to the original
// source.  This works, but is not elegant.  I am considering two alternatives.
//    1: Each source will keep a list of the objects that have made connections
// to it.  This creates a doubley linked list that allows forward propagation
// of PipelineModified messages.  The PipelineMTime will always be upto date.
//    2: After a UpdateRegion call returns, the PipelineMTime will be correct.
// This is similar to the way the Update message works in VTK.  
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
// Set the Input of a filter. If a DataType has not been set for this filter,
// then the DataType of the input is used.
void vtkImageFilter::SetInput(vtkImageSource *input)
{
  vtkDebugMacro(<< "SetInput: input = " << input->GetClassName()
		<< " (" << input << ")");

  // does this change anything?
  if (input == this->Input)
    {
    return;
    }
  
  this->Input = input;
  this->Modified();

  // Should we use the data type from the input?
  this->CheckCache();      // make sure a cache exists
  if (this->Output->GetDataType() == VTK_IMAGE_VOID)
    {
    this->Output->SetDataType(input->GetDataType());
    if (this->Output->GetDataType() == VTK_IMAGE_VOID)
      {
      vtkErrorMacro(<< "SetInput: Cannot determine DataType of input.");
      }
    }
}



//----------------------------------------------------------------------------
// Description:
// This method gets the input tile necessary to generate the Region,
// gets the output tile from the cache, and executes the filter.
// If the input region generate fails, the region of interest is broken 
// into smaller more manigable pieces.
// Any subclass that over rides this default function must handle
// input generate failures itself.
// Allocating the ouput region is guaranteed to succeed.
// outBBox is not modified or deleted.
void vtkImageFilter::UpdateRegion(vtkImageRegion *outRegion)
{
  vtkImageRegion *inRegion;
  
  if (this->Debug)
    {
    int *b = outRegion->GetBounds4d();
    cerr << "Debug: In " __FILE__ << ", line " << __LINE__ << "\n" 
	 << this->GetClassName() << " (" << this << "): "
	 << "GenerateRegion: " << b[0] << "," << b[1] << ", "
	 << b[2] << "," << b[3] << ", " 
	 << b[4] << "," << b[5] << ", "
	 << b[6] << "," << b[7] << "\n\n";
    }
  
  // To avoid doing this for each execute1d ...
  this->UpdateImageInformation(outRegion);  // probably already has ImageBounds
  
  // If outBBox is empty return imediately.
  if (outRegion->IsEmpty())
    {
    return;
    }
    
  // Make sure the Input has been set.
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Input is not set.");
    return;
    }
  
  // Determine whether to use the execute methods or the generate methods.
  if ( ! this->UseExecuteMethod)
    {
    this->UpdateRegion5d(outRegion);
    return;
    }
  
  // Make the input region that will be used to generate the output region
  inRegion = new vtkImageRegion;
  
  // Fill in image information
  this->Input->UpdateImageInformation(inRegion);
  
  // Translate to local coordinate system
  inRegion->SetAxes(this->Axes);
  
  // Compute the required input region bounds.
  // Copy to fill in bounds of extra dimensions.
  inRegion->SetBounds(outRegion->GetBounds());
  this->ComputeRequiredInputRegionBounds(outRegion, inRegion);

  // Use the input to fill the data of the region.
  this->Input->UpdateRegion(inRegion);
  
  // Make sure the region was not too large 
  if ( ! inRegion->IsAllocated())
    {
    // Call alternative slower generate that breaks the task into pieces 
    inRegion->Delete();
    this->UpdateRegionTiled(outRegion);
    return;
    }
  
  // Get the output region from the cache (guaranteed to succeed).
  this->Output->AllocateRegion(outRegion);

  // fill the output region 
  this->Execute5d(inRegion, outRegion);

  // free the input region
  inRegion->Delete();
}


//----------------------------------------------------------------------------
// Description:
// This method gets the boundary of the input then computes and returns 
// the boundary of the largest region that can be generated. 
void vtkImageFilter::UpdateImageInformation(vtkImageRegion *region)
{
  // Make sure the Input has been set.
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "UpdateImageInformation: Input is not set.");
    return;
    }
  
  this->Input->UpdateImageInformation(region);
  this->ComputeOutputImageInformation(region, region);
}



//----------------------------------------------------------------------------
// Description:
// This method is passed an inRegion that holds the image information
// (image bounds ...) of this filters input, and fills outRegion with
// the image information after this filter is finished.
// outImage is identical to inImage when this method is envoked, and
// outImage may be the same object as in image.
void vtkImageFilter::ComputeOutputImageInformation(vtkImageRegion *inRegion,
						   vtkImageRegion *outRegion)
{
  // Default: Image information does not change (do nothing).
  // Avoid warnings
  inRegion = inRegion;
  outRegion = outRegion;
}



//----------------------------------------------------------------------------
// Description:
// This method computes the bounds of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// bounds of the output region.  After this method finishes, "region" should 
// have the bounds of the required input region.  The default method assumes
// the required input bounds are the same as the output bounds.
// Note: The splitting methods call this method with outRegion = inRegion.
void 
vtkImageFilter::ComputeRequiredInputRegionBounds(vtkImageRegion *outRegion,
						 vtkImageRegion *inRegion)
{
  inRegion->SetBounds(outRegion->GetBounds());
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a 5d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute5d
// method breaks the 5d regions into 4d "images".  The regions have been
// converted to this filters coordinates before this method is called.
void vtkImageFilter::Execute5d(vtkImageRegion *inRegion, 
			       vtkImageRegion *outRegion)
{
  int coordinate4, min4, max4;
  int inBounds[10], outBounds[10];
  
  // Get the bounds of the forth dimension to be eliminated.
  inRegion->GetBounds5d(inBounds);
  outRegion->GetBounds5d(outBounds);

  // This method assumes that the third axis of in and out have same bounds.
  min4 = inBounds[8];
  max4 = inBounds[9];
  if (min4 != outBounds[8] || max4 != outBounds[9]) 
    {
    vtkErrorMacro(<< "Execute5d: Cannot break 5d images into 4d images.");
    return;
    }
  
  // loop over 4d volumes
  for (coordinate4 = min4; coordinate4 <= max4; ++coordinate4)
    {
    // set up the 4d regions.
    inRegion->SetDefaultCoordinate4(coordinate4);
    outRegion->SetDefaultCoordinate4(coordinate4);
    this->Execute4d(inRegion, outRegion);
    }
}
  
  

//----------------------------------------------------------------------------
// Description:
// This method is passed a 4d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute4d
// method breaks the 4d regions into 3d volumes.  The regions have been
// converted to this filters coordinates before this method is called.
void vtkImageFilter::Execute4d(vtkImageRegion *inRegion, 
			       vtkImageRegion *outRegion)
{
  int coordinate3, min3, max3;
  int inBounds[8], outBounds[8];
  
  // Get the bounds of the third dimension to be eliminated.
  inRegion->GetBounds4d(inBounds);
  outRegion->GetBounds4d(outBounds);

  // This method assumes that the third axis of in and out have same bounds.
  min3 = inBounds[6];
  max3 = inBounds[7];
  if (min3 != outBounds[6] || max3 != outBounds[7]) 
    {
    vtkErrorMacro(<< "Execute4d: Cannot break 4d images into volumes.");
    return;
    }
  
  // loop over 3d volumes
  for (coordinate3 = min3; coordinate3 <= max3; ++coordinate3)
    {
    // set up the 3d regions.
    inRegion->SetDefaultCoordinate3(coordinate3);
    outRegion->SetDefaultCoordinate3(coordinate3);
    this->Execute3d(inRegion, outRegion);
    }
}
  
  

//----------------------------------------------------------------------------
// Description:
// This method is passed a 3d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute3d
// method breaks the volumes into images.  The regions have been converted to
// this filters coordinates before this method is called.
void vtkImageFilter::Execute3d(vtkImageRegion *inRegion, 
			       vtkImageRegion *outRegion)
{
  int coordinate2, min2, max2;
  int inBounds[6], outBounds[6];
  
  // Get the bounds of the third dimension to be eliminated.
  inRegion->GetBounds3d(inBounds);
  outRegion->GetBounds3d(outBounds);

  // This method assumes that the third axis of in and out have same bounds.
  min2 = inBounds[4];
  max2 = inBounds[5];
  if (min2 != outBounds[4] || max2 != outBounds[5]) 
    {
    vtkErrorMacro(<< "Execute3d: Cannot break volumes into images.");
    return;
    }
  
  // loop over 2d images
  for (coordinate2 = min2; coordinate2 <= max2; ++coordinate2)
    {
    // set up the 2d regions.
    inRegion->SetDefaultCoordinate2(coordinate2);
    outRegion->SetDefaultCoordinate2(coordinate2);
    this->Execute2d(inRegion, outRegion);
    }
}
  
  

//----------------------------------------------------------------------------
// Description:
// This method is passed a 2d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute2d
// method breaks the images into lines.  The regions have been converted to
// this filters coordinates before this method is called.
void vtkImageFilter::Execute2d(vtkImageRegion *inRegion, 
			       vtkImageRegion *outRegion)
{
  int coordinate1, min1, max1;
  int inBounds[4], outBounds[4];
  
  // Get the bounds of the third dimension to be eliminated.
  inRegion->GetBounds2d(inBounds);
  outRegion->GetBounds2d(outBounds);

  // This method assumes that the second axis of in and out have same bounds.
  min1 = inBounds[2];
  max1 = inBounds[3];
  if (min1 != outBounds[2] || max1 != outBounds[3]) 
    {
    vtkErrorMacro(<< "Execute2d: Cannot break images into lines.");
    return;
    }
  
  // loop over 1d lines
  for (coordinate1 = min1; coordinate1 <= max1; ++coordinate1)
    {
    // set up the 1d regions.
    inRegion->SetDefaultCoordinate1(coordinate1);
    outRegion->SetDefaultCoordinate1(coordinate1);
    this->Execute1d(inRegion, outRegion);
    }
}
  
  
//----------------------------------------------------------------------------
// Description:
// This method is passed a 2d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute2d
// method breaks the images into lines.  The regions have been converted to
// this filters coordinates before this method is called.
void vtkImageFilter::Execute1d(vtkImageRegion *inRegion, 
			       vtkImageRegion *outRegion)
{
  inRegion = inRegion;
  outRegion = outRegion;
  
  vtkErrorMacro(<< "Execute1d: Filter does not specify an execute method.");
}



/*****************************************************************************
  Stuff for executing the filter in pieces.
*****************************************************************************/

//----------------------------------------------------------------------------
// Description:
// This method generates the out Region in pieces.
void vtkImageFilter::UpdateRegionTiled(vtkImageRegion *outRegion)
{
  int genericPieceSize[VTK_IMAGE_DIMENSIONS];     
  int pieceBounds[VTK_IMAGE_BOUNDS_DIMENSIONS];
  int outBounds[VTK_IMAGE_BOUNDS_DIMENSIONS];

  vtkDebugMacro(<< "GenerateRegionTiled: outRegion must be split into pieces");

  // Have we bottomed out splitting the tile and gotten to one pixel?
  if (outRegion->GetVolume() <= 1)
    {
    vtkErrorMacro(<< "Cannot split any more. (outRegion is only one pixel)");
    return;
    }

  // choose a piece of the outRegion to determine how the tile is divided 
  // pieces near the edge of the tile can be smaller than this generic piece 
  this->SplitRegion(outRegion, genericPieceSize);

  // loop over the output Region generating the pieces 
  outRegion->GetBounds4d(outBounds);
  // loop over the Components dimensions
  pieceBounds[8] = outBounds[8];
  while (pieceBounds[8] <= outBounds[9])
    {
    pieceBounds[9] = pieceBounds[8] + genericPieceSize[4] - 1;
    // make the piece smaller if it extends over edge
    if (pieceBounds[9] > outBounds[9])
      {
      pieceBounds[9] = outBounds[9];
      }
    // loop over the TIME dimensions
    pieceBounds[6] = outBounds[6];
    while (pieceBounds[6] <= outBounds[7])
      {
      pieceBounds[7] = pieceBounds[6] + genericPieceSize[3] - 1;
      // make the piece smaller if it extends over edge
      if (pieceBounds[7] > outBounds[7])
	{
	pieceBounds[7] = outBounds[7];
	}
      // loop over the Z dimensions
      pieceBounds[4] = outBounds[4];
      while (pieceBounds[4] <= outBounds[5])
	{
	pieceBounds[5] = pieceBounds[4] + genericPieceSize[2] - 1;
	// make the piece smaller if it extends over edge
	if (pieceBounds[5] > outBounds[5])
	  {
	  pieceBounds[5] = outBounds[5];
	  }
	// loop over the Y dimensions
	pieceBounds[2] = outBounds[2];
	while (pieceBounds[2] <= outBounds[3])
	  {
	  pieceBounds[3] = pieceBounds[2] + genericPieceSize[1] - 1;
	  // make the piece smaller if it extends over edge
	  if (pieceBounds[3] > outBounds[3])
	    {
	    pieceBounds[3] = outBounds[3];
	    }
	  // loop over the time dimensions
	  pieceBounds[0] = outBounds[0];
	  while (pieceBounds[0] <= outBounds[1])
	    {
	    pieceBounds[1] = pieceBounds[0] + genericPieceSize[0] - 1;
	    // make the piece smaller if it extends over edge
	    if (pieceBounds[1] > outBounds[1])
	      {
	      pieceBounds[1] = outBounds[1];
	      }
	    
	    // Generate the data for this piece
	    outRegion->SetBounds4d(pieceBounds);
	    this->UpdateRegion(outRegion);
	    
	    pieceBounds[0] += genericPieceSize[0];
	    }
	  pieceBounds[2] += genericPieceSize[1];
	  }
	pieceBounds[4] += genericPieceSize[2];
	}
      pieceBounds[6] += genericPieceSize[3];
      }
    pieceBounds[8] += genericPieceSize[4];
    }
  
  // reset the original bounds of the region
  outRegion->SetBounds(outBounds);
}



//----------------------------------------------------------------------------
// Description:
// This method is called when the output Region is too large to generate.
// It specifies how to split the Region into pieces by returning a generic
// pieceSize.  Over ride this method if you have a specific way of breaking 
// up a tile that is more efficient than this default.
void vtkImageFilter::SplitRegion(vtkImageRegion *outRegion, int *pieceSize)
{
  int outBounds[VTK_IMAGE_BOUNDS_DIMENSIONS];
  int newBounds[VTK_IMAGE_BOUNDS_DIMENSIONS];
  int size;
  int memory;
  int bestMemory;

  outRegion->GetBounds(outBounds);
  outRegion->GetBounds(newBounds);
  
  // split down the first axis (two pieces, round down) (keep middle)
  size = (outBounds[1] - outBounds[0] + 1) / 2;
  if ( size > 3)    // avoid remainder slivers
    {
    ++size;  
    }
  newBounds[0] = outBounds[0] + size / 2;
  newBounds[1] = newBounds[0] + size;
  
  // determine the input Region for this newSize 
  outRegion->SetBounds(newBounds);
  this->ComputeRequiredInputRegionBounds(outRegion, outRegion);
  // memory needed 
  memory = outRegion->GetVolume();
  // save the best (smallest memory) so far 
  bestMemory = memory;
  pieceSize[0]=newBounds[0];  
  pieceSize[1]=newBounds[2];  
  pieceSize[2]=newBounds[4];
  pieceSize[3]=newBounds[6];
  pieceSize[4]=newBounds[8];

  // Reset the size of the first axis
  newBounds[0] = outBounds[0];  newBounds[1] = outBounds[1];
  // split down the second axis (two pieces, round down) (keep middle)
  size = (outBounds[3] - outBounds[2] + 1) / 2;
  if ( size > 3)    // avoid remainder slivers
    {
    ++size;  
    }
  newBounds[2] = outBounds[2] + size / 2;
  newBounds[3] = newBounds[2] + size;
  
  // determine the input Region for this newSize 
  outRegion->SetBounds(newBounds);
  this->ComputeRequiredInputRegionBounds(outRegion, outRegion);
  // memory needed 
  memory = outRegion->GetVolume();
  // save the best (smallest memory) so far 
  if (memory < bestMemory)
    {
    bestMemory = memory;
    pieceSize[0]=newBounds[0];  
    pieceSize[1]=newBounds[2];  
    pieceSize[2]=newBounds[4];
    pieceSize[3]=newBounds[6];
    pieceSize[4]=newBounds[8];
    }
  
  // Reset the size of the second axis
  newBounds[2] = outBounds[2];   newBounds[3] = outBounds[3];
  // split down the third axis (two pieces, round down) (keep middle)
  size = (outBounds[5] - outBounds[4] + 1) / 2;
  if ( size > 3)    // avoid remainder slivers
    {
    ++size;  
    }
  newBounds[4] = outBounds[4] + size / 2;
  newBounds[5] = newBounds[4] + size;
  
  // determine the input Region for this newSize 
  outRegion->SetBounds(newBounds);
  this->ComputeRequiredInputRegionBounds(outRegion, outRegion);
  // memory needed 
  memory = outRegion->GetVolume();
  // save the best (smallest memory) so far 
  if (memory < bestMemory)
    {
    bestMemory = memory;
    pieceSize[0]=newBounds[0];  
    pieceSize[1]=newBounds[2];  
    pieceSize[2]=newBounds[4];
    pieceSize[3]=newBounds[6];
    pieceSize[4]=newBounds[8];
    }
  
  // Reset the size of the third axis
  newBounds[4] = outBounds[4];   newBounds[5] = outBounds[5];
  // split down the fourth axis (two pieces, round down) (keep middle)
  size = (outBounds[7] - outBounds[6] + 1) / 2;
  if ( size > 3 )    // avoid remainder slivers
    {
    ++size;  
    }
  newBounds[6] = outBounds[6] + size / 2;
  newBounds[7] = newBounds[6] + size;
  
  // determine the input Region for this newSize 
  outRegion->SetBounds(newBounds);
  this->ComputeRequiredInputRegionBounds(outRegion, outRegion);
  // memory needed 
  memory = outRegion->GetVolume();
  // save the best (smallest memory) so far 
  if (memory < bestMemory)
    {
    bestMemory = memory;
    pieceSize[0]=newBounds[0];  
    pieceSize[1]=newBounds[2];  
    pieceSize[2]=newBounds[4];
    pieceSize[3]=newBounds[6];
    pieceSize[4]=newBounds[8];
    }
  
  // Reset the size of the forth axis
  newBounds[6] = outBounds[6];   newBounds[7] = outBounds[7];
  // split down the fifth axis (two pieces, round down) (keep middle)
  size = (outBounds[9] - outBounds[8] + 1) / 2;
  if ( size > 3 )    // avoid remainder slivers
    {
    ++size;  
    }
  newBounds[8] = outBounds[8] + size / 2;
  newBounds[9] = newBounds[8] + size;
  
  // determine the input Region for this newSize 
  outRegion->SetBounds(newBounds);
  this->ComputeRequiredInputRegionBounds(outRegion, outRegion);
  // memory needed 
  memory = outRegion->GetVolume();
  // save the best (smallest memory) so far 
  if (memory < bestMemory)
    {
    bestMemory = memory;
    pieceSize[0]=newBounds[0];  
    pieceSize[1]=newBounds[2];  
    pieceSize[2]=newBounds[4];
    pieceSize[3]=newBounds[6];
    pieceSize[4]=newBounds[8];
    }
  
  // Reset the bounds of the region
  outRegion->SetBounds(outBounds);
}


//============================================================================
// Stuff for filters that do not use the execute methods..
//============================================================================

//----------------------------------------------------------------------------
vtkImageRegion *vtkImageFilter::GetInputRegion(int *bounds, int dim)
{
  int idx;
  int *imageBounds;
  vtkImageRegion *region;
  
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Input is not set.");
    return NULL;
    }

  region = new vtkImageRegion;

  // This step is just error checking, and may be wastefull.  The Image
  // Information is automatically computed when UpdateRegion is called.
  this->Input->UpdateImageInformation(region);
  region->SetAxes(this->GetAxes());
  imageBounds = region->GetImageBounds();
  for (idx = dim; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (imageBounds[idx*2] > 0 || imageBounds[idx*2 + 1] < 0)
      {
      vtkErrorMacro(<< "GetInputRegion: dim = " << dim 
                    << ", unspecified dimensions do not include 0.");
      region->Delete();
      return NULL;
      }
    }
  
  // Note: This automatical sets the unspecified dimension bounds to [0,0]
  region->SetBounds(bounds, dim);
  this->Input->UpdateRegion(region);
  
  return region;
}














