/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFilter.cxx
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
#include "vtkImageFilter.h"
#include "vtkImageCache.h"


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
    int *b = outRegion->GetExtent4d();
    cerr << "Debug: In " __FILE__ << ", line " << __LINE__ << "\n" 
	 << this->GetClassName() << " (" << this << "): "
	 << "GenerateRegion: " << b[0] << "," << b[1] << ", "
	 << b[2] << "," << b[3] << ", " 
	 << b[4] << "," << b[5] << ", "
	 << b[6] << "," << b[7] << "\n\n";
    }
  
  // To avoid doing this for each execute1d ...
  this->UpdateImageInformation(outRegion);  // probably already has ImageExtent
  
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
  
  // Compute the required input region extent.
  // Copy to fill in extent of extra dimensions.
  inRegion->SetExtent(outRegion->GetExtent());
  this->ComputeRequiredInputRegionExtent(outRegion, inRegion);

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
// (image extent ...) of this filters input, and fills outRegion with
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
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// extent of the output region.  After this method finishes, "region" should 
// have the extent of the required input region.  The default method assumes
// the required input extent are the same as the output extent.
// Note: The splitting methods call this method with outRegion = inRegion.
void 
vtkImageFilter::ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
						 vtkImageRegion *inRegion)
{
  inRegion->SetExtent(outRegion->GetExtent());
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
  int inExtent[10], outExtent[10];
  
  // Get the extent of the forth dimension to be eliminated.
  inRegion->GetExtent5d(inExtent);
  outRegion->GetExtent5d(outExtent);

  // This method assumes that the third axis of in and out have same extent.
  min4 = inExtent[8];
  max4 = inExtent[9];
  if (min4 != outExtent[8] || max4 != outExtent[9]) 
    {
    vtkErrorMacro(<< "Execute5d: Cannot break 5d images into 4d images.");
    return;
    }
  
  // loop over 4d volumes
  for (coordinate4 = min4; coordinate4 <= max4; ++coordinate4)
    {
    // set up the 4d regions.
    inExtent[8] = coordinate4;
    inExtent[9] = coordinate4;
    inRegion->SetExtent5d(inExtent);
    outExtent[8] = coordinate4;
    outExtent[9] = coordinate4;
    outRegion->SetExtent5d(outExtent);
    this->Execute4d(inRegion, outRegion);
    }
  // restore the original extent
  inExtent[8] = min4;
  inExtent[9] = max4;
  outExtent[8] = min4;
  outExtent[9] = max4; 
  inRegion->SetExtent5d(inExtent);
  outRegion->SetExtent5d(outExtent);
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
  int inExtent[8], outExtent[8];
  
  // Get the extent of the third dimension to be eliminated.
  inRegion->GetExtent4d(inExtent);
  outRegion->GetExtent4d(outExtent);

  // This method assumes that the third axis of in and out have same extent.
  min3 = inExtent[6];
  max3 = inExtent[7];
  if (min3 != outExtent[6] || max3 != outExtent[7]) 
    {
    vtkErrorMacro(<< "Execute4d: Cannot break 4d images into volumes.");
    return;
    }
  
  // loop over 3d volumes
  for (coordinate3 = min3; coordinate3 <= max3; ++coordinate3)
    {
    // set up the 3d regions.
    inExtent[6] = coordinate3;
    inExtent[7] = coordinate3;
    inRegion->SetExtent4d(inExtent);
    outExtent[6] = coordinate3;
    outExtent[7] = coordinate3;
    outRegion->SetExtent4d(outExtent);
    this->Execute3d(inRegion, outRegion);
    }
  // restore the original extent
  inExtent[6] = min3;
  inExtent[7] = max3;
  outExtent[6] = min3;
  outExtent[7] = max3; 
  inRegion->SetExtent4d(inExtent);
  outRegion->SetExtent4d(outExtent);
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
  int inExtent[6], outExtent[6];
  
  // Get the extent of the third dimension to be eliminated.
  inRegion->GetExtent3d(inExtent);
  outRegion->GetExtent3d(outExtent);

  // This method assumes that the third axis of in and out have same extent.
  min2 = inExtent[4];
  max2 = inExtent[5];
  if (min2 != outExtent[4] || max2 != outExtent[5]) 
    {
    vtkErrorMacro(<< "Execute3d: Cannot break volumes into images.");
    return;
    }
  
  // loop over 2d images
  for (coordinate2 = min2; coordinate2 <= max2; ++coordinate2)
    {
    // set up the 2d regions.
    inExtent[4] = coordinate2;
    inExtent[5] = coordinate2;
    inRegion->SetExtent3d(inExtent);
    outExtent[4] = coordinate2;
    outExtent[5] = coordinate2;
    outRegion->SetExtent3d(outExtent);
    this->Execute2d(inRegion, outRegion);
    }
  // restore the original extent
  inExtent[4] = min2;
  inExtent[5] = max2;
  outExtent[4] = min2;
  outExtent[5] = max2; 
  inRegion->SetExtent3d(inExtent);
  outRegion->SetExtent3d(outExtent);
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
  int inExtent[4], outExtent[4];
  
  // Get the extent of the third dimension to be eliminated.
  inRegion->GetExtent2d(inExtent);
  outRegion->GetExtent2d(outExtent);

  // This method assumes that the second axis of in and out have same extent.
  min1 = inExtent[2];
  max1 = inExtent[3];
  if (min1 != outExtent[2] || max1 != outExtent[3]) 
    {
    vtkErrorMacro(<< "Execute2d: Cannot break images into lines.");
    return;
    }
  
  // loop over 1d lines
  for (coordinate1 = min1; coordinate1 <= max1; ++coordinate1)
    {
    // set up the 1d regions.
    inExtent[2] = coordinate1;
    inExtent[3] = coordinate1;
    inRegion->SetExtent2d(inExtent);
    outExtent[2] = coordinate1;
    outExtent[3] = coordinate1;
    outRegion->SetExtent2d(outExtent);
    this->Execute1d(inRegion, outRegion);
    }
  // restore the original extent
  inExtent[2] = min1;
  inExtent[3] = max1;
  outExtent[2] = min1;
  outExtent[3] = max1; 
  inRegion->SetExtent2d(inExtent);
  outRegion->SetExtent2d(outExtent);
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
  int pieceExtent[VTK_IMAGE_BOUNDS_DIMENSIONS];
  int outExtent[VTK_IMAGE_BOUNDS_DIMENSIONS];

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
  outRegion->GetExtent4d(outExtent);
  // loop over the Components dimensions
  pieceExtent[8] = outExtent[8];
  while (pieceExtent[8] <= outExtent[9])
    {
    pieceExtent[9] = pieceExtent[8] + genericPieceSize[4] - 1;
    // make the piece smaller if it extends over edge
    if (pieceExtent[9] > outExtent[9])
      {
      pieceExtent[9] = outExtent[9];
      }
    // loop over the TIME dimensions
    pieceExtent[6] = outExtent[6];
    while (pieceExtent[6] <= outExtent[7])
      {
      pieceExtent[7] = pieceExtent[6] + genericPieceSize[3] - 1;
      // make the piece smaller if it extends over edge
      if (pieceExtent[7] > outExtent[7])
	{
	pieceExtent[7] = outExtent[7];
	}
      // loop over the Z dimensions
      pieceExtent[4] = outExtent[4];
      while (pieceExtent[4] <= outExtent[5])
	{
	pieceExtent[5] = pieceExtent[4] + genericPieceSize[2] - 1;
	// make the piece smaller if it extends over edge
	if (pieceExtent[5] > outExtent[5])
	  {
	  pieceExtent[5] = outExtent[5];
	  }
	// loop over the Y dimensions
	pieceExtent[2] = outExtent[2];
	while (pieceExtent[2] <= outExtent[3])
	  {
	  pieceExtent[3] = pieceExtent[2] + genericPieceSize[1] - 1;
	  // make the piece smaller if it extends over edge
	  if (pieceExtent[3] > outExtent[3])
	    {
	    pieceExtent[3] = outExtent[3];
	    }
	  // loop over the time dimensions
	  pieceExtent[0] = outExtent[0];
	  while (pieceExtent[0] <= outExtent[1])
	    {
	    pieceExtent[1] = pieceExtent[0] + genericPieceSize[0] - 1;
	    // make the piece smaller if it extends over edge
	    if (pieceExtent[1] > outExtent[1])
	      {
	      pieceExtent[1] = outExtent[1];
	      }
	    
	    // Generate the data for this piece
	    outRegion->SetExtent4d(pieceExtent);
	    this->UpdateRegion(outRegion);
	    
	    pieceExtent[0] += genericPieceSize[0];
	    }
	  pieceExtent[2] += genericPieceSize[1];
	  }
	pieceExtent[4] += genericPieceSize[2];
	}
      pieceExtent[6] += genericPieceSize[3];
      }
    pieceExtent[8] += genericPieceSize[4];
    }
  
  // reset the original extent of the region
  outRegion->SetExtent(outExtent);
}



//----------------------------------------------------------------------------
// Description:
// This method is called when the output Region is too large to generate.
// It specifies how to split the Region into pieces by returning a generic
// pieceSize.  Over ride this method if you have a specific way of breaking 
// up a tile that is more efficient than this default.
void vtkImageFilter::SplitRegion(vtkImageRegion *outRegion, int *pieceSize)
{
  int outExtent[VTK_IMAGE_BOUNDS_DIMENSIONS];
  int newExtent[VTK_IMAGE_BOUNDS_DIMENSIONS];
  int size;
  int memory;
  int bestMemory;

  outRegion->GetExtent(outExtent);
  outRegion->GetExtent(newExtent);
  
  // split down the first axis (two pieces, round down) (keep middle)
  size = (outExtent[1] - outExtent[0] + 1) / 2;
  if ( size > 3)    // avoid remainder slivers
    {
    ++size;  
    }
  newExtent[0] = outExtent[0] + size / 2;
  newExtent[1] = newExtent[0] + size;
  
  // determine the input Region for this newSize 
  outRegion->SetExtent(newExtent);
  this->ComputeRequiredInputRegionExtent(outRegion, outRegion);
  // memory needed 
  memory = outRegion->GetVolume();
  // save the best (smallest memory) so far 
  bestMemory = memory;
  pieceSize[0]=newExtent[0];  
  pieceSize[1]=newExtent[2];  
  pieceSize[2]=newExtent[4];
  pieceSize[3]=newExtent[6];
  pieceSize[4]=newExtent[8];

  // Reset the size of the first axis
  newExtent[0] = outExtent[0];  newExtent[1] = outExtent[1];
  // split down the second axis (two pieces, round down) (keep middle)
  size = (outExtent[3] - outExtent[2] + 1) / 2;
  if ( size > 3)    // avoid remainder slivers
    {
    ++size;  
    }
  newExtent[2] = outExtent[2] + size / 2;
  newExtent[3] = newExtent[2] + size;
  
  // determine the input Region for this newSize 
  outRegion->SetExtent(newExtent);
  this->ComputeRequiredInputRegionExtent(outRegion, outRegion);
  // memory needed 
  memory = outRegion->GetVolume();
  // save the best (smallest memory) so far 
  if (memory < bestMemory)
    {
    bestMemory = memory;
    pieceSize[0]=newExtent[0];  
    pieceSize[1]=newExtent[2];  
    pieceSize[2]=newExtent[4];
    pieceSize[3]=newExtent[6];
    pieceSize[4]=newExtent[8];
    }
  
  // Reset the size of the second axis
  newExtent[2] = outExtent[2];   newExtent[3] = outExtent[3];
  // split down the third axis (two pieces, round down) (keep middle)
  size = (outExtent[5] - outExtent[4] + 1) / 2;
  if ( size > 3)    // avoid remainder slivers
    {
    ++size;  
    }
  newExtent[4] = outExtent[4] + size / 2;
  newExtent[5] = newExtent[4] + size;
  
  // determine the input Region for this newSize 
  outRegion->SetExtent(newExtent);
  this->ComputeRequiredInputRegionExtent(outRegion, outRegion);
  // memory needed 
  memory = outRegion->GetVolume();
  // save the best (smallest memory) so far 
  if (memory < bestMemory)
    {
    bestMemory = memory;
    pieceSize[0]=newExtent[0];  
    pieceSize[1]=newExtent[2];  
    pieceSize[2]=newExtent[4];
    pieceSize[3]=newExtent[6];
    pieceSize[4]=newExtent[8];
    }
  
  // Reset the size of the third axis
  newExtent[4] = outExtent[4];   newExtent[5] = outExtent[5];
  // split down the fourth axis (two pieces, round down) (keep middle)
  size = (outExtent[7] - outExtent[6] + 1) / 2;
  if ( size > 3 )    // avoid remainder slivers
    {
    ++size;  
    }
  newExtent[6] = outExtent[6] + size / 2;
  newExtent[7] = newExtent[6] + size;
  
  // determine the input Region for this newSize 
  outRegion->SetExtent(newExtent);
  this->ComputeRequiredInputRegionExtent(outRegion, outRegion);
  // memory needed 
  memory = outRegion->GetVolume();
  // save the best (smallest memory) so far 
  if (memory < bestMemory)
    {
    bestMemory = memory;
    pieceSize[0]=newExtent[0];  
    pieceSize[1]=newExtent[2];  
    pieceSize[2]=newExtent[4];
    pieceSize[3]=newExtent[6];
    pieceSize[4]=newExtent[8];
    }
  
  // Reset the size of the forth axis
  newExtent[6] = outExtent[6];   newExtent[7] = outExtent[7];
  // split down the fifth axis (two pieces, round down) (keep middle)
  size = (outExtent[9] - outExtent[8] + 1) / 2;
  if ( size > 3 )    // avoid remainder slivers
    {
    ++size;  
    }
  newExtent[8] = outExtent[8] + size / 2;
  newExtent[9] = newExtent[8] + size;
  
  // determine the input Region for this newSize 
  outRegion->SetExtent(newExtent);
  this->ComputeRequiredInputRegionExtent(outRegion, outRegion);
  // memory needed 
  memory = outRegion->GetVolume();
  // save the best (smallest memory) so far 
  if (memory < bestMemory)
    {
    bestMemory = memory;
    pieceSize[0]=newExtent[0];  
    pieceSize[1]=newExtent[2];  
    pieceSize[2]=newExtent[4];
    pieceSize[3]=newExtent[6];
    pieceSize[4]=newExtent[8];
    }
  
  // Reset the extent of the region
  outRegion->SetExtent(outExtent);
}


//============================================================================
// Stuff for filters that do not use the execute methods..
//============================================================================

//----------------------------------------------------------------------------
vtkImageRegion *vtkImageFilter::GetInputRegion(int *extent, int dim)
{
  int idx;
  int *imageExtent;
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
  imageExtent = region->GetImageExtent();
  for (idx = dim; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (imageExtent[idx*2] > 0 || imageExtent[idx*2 + 1] < 0)
      {
      vtkErrorMacro(<< "GetInputRegion: dim = " << dim 
                    << ", unspecified dimensions do not include 0.");
      region->Delete();
      return NULL;
      }
    }
  
  // Note: This automatical sets the unspecified dimension extent to [0,0]
  region->SetExtent(extent, dim);
  this->Input->UpdateRegion(region);
  
  return region;
}














