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
  this->UseExecuteMethod = 1;
  this->InputMemoryLimit = 100000; // 100 MBytes
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
  os << indent << "InputMemoryLimit: " << this->InputMemoryLimit << "\n";
}

//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline upto and including this filter
// Note: current implementation may create a cascade of GetPipelineMTime calls.
// Each GetPipelineMTime call propagates the call all the way to the original
// source.  
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
// Set the Input of a filter. If a ScalarType has not been set for this filter,
// then the ScalarType of the input is used.
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
  if (this->Output->GetScalarType() == VTK_VOID)
    {
    this->Output->SetScalarType(input->GetScalarType());
    if (this->Output->GetScalarType() == VTK_VOID)
      {
      vtkErrorMacro(<< "SetInput: Cannot determine ScalarType of input.");
      }
    }
}


//----------------------------------------------------------------------------
// Description:
// This method is called by the cache.  It calls the
// UpdatePointData(vtkImageRegion *) method or the 
// Execute(vtkImageRegion *, vtkImageRegion *) method depending of whether
// UseExecuteMethod is on.  ImageInformation has already been
// updated by this point, and outRegion is in local coordinates.
void vtkImageFilter::UpdatePointData(int dim, vtkImageRegion *outRegion)
{
  vtkImageRegion *inRegion;

  // If outBBox is empty return imediately.
  if (outRegion->IsEmpty())
    {
    return;
    }
    
  // Determine whether to use the execute methods or the generate methods.
  // It may be useful (in the future) to switch to the execute function
  // at some middle axesIdx.  Streaming would result.
  if ( ! this->UseExecuteMethod)
    {
    this->vtkImageCachedSource::UpdatePointData(dim, outRegion);
    return;
    }
  
  // Make sure the Input has been set.
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Input is not set.");
    return;
    }
  
  // Make the input region that will be used to generate the output region
  inRegion = new vtkImageRegion;
  // Fill in image information (ComputeRequiredInputExtent may need it)
  this->Input->UpdateImageInformation(inRegion);
  // Set the coordinate system
  inRegion->SetAxes(VTK_IMAGE_DIMENSIONS, this->Axes);
  
  // Compute the required input region extent.
  // Copy to fill in extent of extra dimensions.
  inRegion->SetExtent(VTK_IMAGE_DIMENSIONS, outRegion->GetExtent());
  this->ComputeRequiredInputRegionExtent(outRegion, inRegion);

  // Cheap and dirty streaming
  // No split order instance variable, and can not split into two ...
  if (inRegion->GetMemorySize() > this->InputMemoryLimit)
    {
    inRegion->Delete();
    if (dim == 0)
      {
      vtkErrorMacro(<< "UpdatePointData: Memory Limit "
                    << this->InputMemoryLimit << " must be really small");
      }
    else
      {
      this->vtkImageCachedSource::UpdatePointData(dim, outRegion);
      }
    return;
    }
  
  // Use the input to fill the data of the region.
  this->Input->UpdateRegion(inRegion);
  
  // Make sure the region was not too large 
  if ( ! inRegion->AreScalarsAllocated())
    {
    // Try Streaming
    inRegion->Delete();
    if (dim == 0)
      {
      vtkErrorMacro(<< "UpdatePointData: Could not get input.");
      }
    else
      {
      this->vtkImageCachedSource::UpdatePointData(dim, outRegion);
      }
    return;
    }
  
  // Get the output region from the cache (guaranteed to succeed).
  this->Output->AllocateRegion(outRegion);

  // fill the output region 
  this->Execute(dim, inRegion, outRegion);

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
// This execute method recursively loops over extra dimensions and
// calls the subclasses Execute method with lower dimensional regions.
void vtkImageFilter::Execute(int dim, vtkImageRegion *inRegion, 
			     vtkImageRegion *outRegion)
{
  int coordinate, axis;
  int inMin, inMax;
  int outMin, outMax;
  
  
  // Terminate recursion?
  if (dim <= this->NumberOfAxes)
    {
    this->Execute(inRegion, outRegion);
    return;
    }
  
  // Get the extent of the forth dimension to be eliminated.
  axis = this->Axes[dim - 1];
  inRegion->GetAxisExtent(axis, inMin, inMax);
  outRegion->GetAxisExtent(axis, outMin, outMax);

  // The axis should have the same extent.
  if (inMin != outMin || inMax != outMax) 
    {
    vtkErrorMacro(<< "Execute: Extra axis " << vtkImageAxisNameMacro(axis)
    << " can not be eliminated");
    return;
    }
  
  // loop over the samples along the extra axis.
  for (coordinate = inMin; coordinate <= inMax; ++coordinate)
    {
    // set up the lower dimensional regions.
    inRegion->SetAxisExtent(axis, coordinate, coordinate);
    outRegion->SetAxisExtent(axis, coordinate, coordinate);
    this->vtkImageFilter::Execute(dim - 1, inRegion, outRegion);
    }
  // restore the original extent
  inRegion->SetAxisExtent(axis, inMin, inMax);
  outRegion->SetAxisExtent(axis, outMax, outMax);
}




//----------------------------------------------------------------------------
// Description:
// The execute method created by the subclass.
void vtkImageFilter::Execute(vtkImageRegion *inRegion, 
			     vtkImageRegion *outRegion)
{
  inRegion = outRegion;
  vtkErrorMacro(<< "Subclass needs to suply an execute function.");
}

  


//============================================================================
// Stuff for filters that do not use the execute methods..
//============================================================================

//----------------------------------------------------------------------------
vtkImageRegion *vtkImageFilter::GetInputRegion(int dim, int *extent)
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
  
  // Note: This automatically sets the unspecified dimension extent to [0,0]
  region->SetExtent(dim, extent);
  this->Input->UpdateRegion(region);
  
  return region;
}














