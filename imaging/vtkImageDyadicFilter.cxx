/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDyadicFilter.cxx
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
#include "vtkImageDyadicFilter.h"
#include "vtkImageCache.h"


//----------------------------------------------------------------------------
vtkImageDyadicFilter::vtkImageDyadicFilter()
{
  this->Input1 = NULL;
  this->Input2 = NULL;
  this->UseExecuteMethodOn();
  this->InputMemoryLimit = 100000;   // 100 MBytes

  // Invalid
  this->Dimensionality = -1;
  this->ExecuteDimensionality = -1;
}

//----------------------------------------------------------------------------
void vtkImageDyadicFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageCachedSource::PrintSelf(os,indent);
  os << indent << "Input1: (" << this->Input1 << ")\n";
  os << indent << "Input2: (" << this->Input2 << ")\n";
}

//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline upto and including this filter
// Note: current implementation may create a cascade of GetPipelineMTime calls.
// Each GetPipelineMTime call propagates the call all the way to the original
// source.  This works, but is not elegant.
// An Executor would probably be the best solution if this is a problem.
// (The pipeline could vote before it starts processing, but one object
// has to initiate the voting.)
unsigned long int vtkImageDyadicFilter::GetPipelineMTime()
{
  unsigned long int time1, time2;

  // This objects MTime
  // (Super class considers cache in case cache did not originate message)
  time1 = this->vtkImageCachedSource::GetPipelineMTime();
  if ( ! this->Input1 )
    {
    vtkWarningMacro(<< "GetPipelineMTime: Input1 not set.");
    time2 = time1;
    }
  else
    {
    time2 = this->Input1->GetPipelineMTime();
    }
    
  // Keep the larger of the two 
  if (time2 > time1)
    time1 = time2;

  if ( ! this->Input2 )
    {
    vtkWarningMacro(<< "GetPipelineMTime: Input2 not set.");
    return time1;
    }
  time2 = this->Input2->GetPipelineMTime();

  // Keep the larger of the two 
  if (time2 > time1)
    time1 = time2;

  return time1;
}


//----------------------------------------------------------------------------
// Description:
// Set the Input1 of this filter. If a ScalarType has not been set,
// then the ScalarType of the input is used.
void vtkImageDyadicFilter::SetInput1(vtkImageSource *input)
{
  vtkDebugMacro(<< "SetInput1: input = " << input->GetClassName()
		<< " (" << input << ")");

  // does this change anything?
  if (input == this->Input1)
    {
    return;
    }
  
  this->Input1 = input;
  this->Modified();

  // Should we use the data type from the input?
  this->CheckCache();      // make sure a cache exists
  if (this->Output->GetScalarType() == VTK_VOID)
    {
    this->Output->SetScalarType(input->GetScalarType());
    if (this->Output->GetScalarType() == VTK_VOID)
      {
      vtkErrorMacro(<< "SetInput1: Cannot determine ScalarType of input.");
      }
    }
}



//----------------------------------------------------------------------------
// Description:
// Set the Input2 of this filter. If a ScalarType has not been set,
// then the ScalarType of the input is used.
void vtkImageDyadicFilter::SetInput2(vtkImageSource *input)
{
  vtkDebugMacro(<< "SetInput2: input = " << input->GetClassName()
		<< " (" << input << ")");

  // does this change anything?
  if (input == this->Input2)
    {
    return;
    }
  
  this->Input2 = input;
  this->Modified();

  // Should we use the data type from the input?
  this->CheckCache();      // make sure a cache exists
  if (this->Output->GetScalarType() == VTK_VOID)
    {
    this->Output->SetScalarType(input->GetScalarType());
    if (this->Output->GetScalarType() == VTK_VOID)
      {
      vtkErrorMacro(<< "SetInput2: Cannot determine ScalarType of input.");
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
void 
vtkImageDyadicFilter::UpdatePointData(int dim, vtkImageRegion *outRegion)
{
  vtkImageRegion *inRegion1, *inRegion2;
  
  // If outBBox is empty return imediately.
  if (outRegion->IsEmpty())
    {
    return;
    }
    
  // Make sure the Input has been set.
  if ( ! this->Input1 || ! this->Input2)
    {
    vtkErrorMacro(<< "An input is not set.");
    return;
    }
  
  // Determine whether to use the execute methods or the generate methods.
  if ( ! this->UseExecuteMethod)
    {
    this->vtkImageCachedSource::UpdatePointData(dim, outRegion);
    return;
    }
  
  // Make the input regions that will be used to generate the output region
  inRegion1 = new vtkImageRegion;
  inRegion2 = new vtkImageRegion;
  // Fill in image information
  this->Input1->UpdateImageInformation(inRegion1);
  this->Input2->UpdateImageInformation(inRegion2);
  // Translate to local coordinate system
  inRegion1->SetAxes(VTK_IMAGE_DIMENSIONS, this->Axes);
  inRegion2->SetAxes(VTK_IMAGE_DIMENSIONS, this->Axes);
  
  // Compute the required input region extent.
  // Copy to fill in extent of extra dimensions.
  inRegion1->SetExtent(VTK_IMAGE_DIMENSIONS, outRegion->GetExtent());
  inRegion2->SetExtent(VTK_IMAGE_DIMENSIONS, outRegion->GetExtent());
  this->ComputeRequiredInputRegionExtent(outRegion, inRegion1, inRegion2);

  // Streaming not implmented yet. (don't forget to consider scalar type)
  if ((inRegion1->GetVolume() / 1000) > this->InputMemoryLimit || 
      (inRegion2->GetVolume() / 1000) > this->InputMemoryLimit)
    {
    vtkErrorMacro(<< "Streaming not implemented yet.");
    return;
    }
  
  // Use the input to fill the data of the region.
  this->Input1->UpdateRegion(inRegion1);
  this->Input2->UpdateRegion(inRegion2);
  
  // Make sure the region was not too large 
  if ( ! inRegion1->AreScalarsAllocated() || ! inRegion2->AreScalarsAllocated())
    {
    // Try Streaming
    inRegion1->Delete();
    inRegion2->Delete();
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
  
  // fill the output region 
  this->Execute(dim, inRegion1, inRegion2, outRegion);

  // Save the new region in cache.
  this->Output->CacheRegion(outRegion);  
  
  // free the input regions
  inRegion1->Delete();
  inRegion2->Delete();
}


//----------------------------------------------------------------------------
// Description:
// This method gets the boundary of the inputs then computes and returns 
// the boundary of the largest region that can be generated. 
void vtkImageDyadicFilter::UpdateImageInformation(vtkImageRegion *outRegion)
{
  vtkImageRegion *inRegion2;
  
  // Make sure the Input has been set.
  if ( ! this->Input1 || ! this->Input2)
    {
    vtkErrorMacro(<< "UpdateImageInformation: An input is not set.");
    return;
    }

  inRegion2 = new vtkImageRegion;
  
  this->Input1->UpdateImageInformation(outRegion);
  this->Input2->UpdateImageInformation(inRegion2);
  this->ComputeOutputImageInformation(outRegion, inRegion2, outRegion);

  inRegion2->Delete();
}



//----------------------------------------------------------------------------
// Description:
// This method is passed an inRegion that holds the image information
// (image extent ...) of this filters input, and fills outRegion with
// the image information after this filter is finished.
// outImage is identical to inImage when this method is envoked, and
// outImage may be the same object as in image.
void 
vtkImageDyadicFilter::ComputeOutputImageInformation(vtkImageRegion *inRegion1,
						    vtkImageRegion *inRegion2,
						    vtkImageRegion *outRegion)

{
  // Default: Image information does not change (do nothing).
  // Avoid warnings
  inRegion1 = inRegion1;
  inRegion2 = inRegion2;
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
void vtkImageDyadicFilter::ComputeRequiredInputRegionExtent(
					       vtkImageRegion *outRegion,
					       vtkImageRegion *inRegion1,
					       vtkImageRegion *inRegion2)
{
  inRegion1->SetExtent(outRegion->GetExtent());
  inRegion2->SetExtent(outRegion->GetExtent());
}



//----------------------------------------------------------------------------
// Description:
// This execute method recursively loops over extra dimensions and
// calls the subclasses Execute method with lower dimensional regions.
void vtkImageDyadicFilter::Execute(int dim, vtkImageRegion *inRegion1,
				   vtkImageRegion *inRegion2,
				   vtkImageRegion *outRegion)
{
  int coordinate, axis;
  int inMin, inMax;
  int outMin, outMax;
  

  // Terminate recursion?
  if (dim <= this->ExecuteDimensionality)
    {
    this->Execute(inRegion1, inRegion2, outRegion);
    return;
    }
  
  // Get the extent of the forth dimension to be eliminated.
  axis = this->Axes[dim - 1];
  inRegion1->GetAxisExtent(axis, inMin, inMax);
  outRegion->GetAxisExtent(axis, outMin, outMax);

  // Extra axis of in and out must have the same extent
  if (inMin != outMin || inMax != outMax)
    {
    vtkErrorMacro(<< "Execute: Extra axis can not be eliminated.");
    return;
    }
  
  // loop over the samples along the extra axis.
  for (coordinate = inMin; coordinate <= inMax; ++coordinate)
    {
    // set up the lower dimensional regions.
    inRegion1->SetAxisExtent(axis, coordinate, coordinate);
    inRegion2->SetAxisExtent(axis, coordinate, coordinate);
    outRegion->SetAxisExtent(axis, coordinate, coordinate);
    this->Execute(dim - 1, inRegion1, inRegion2, outRegion);
    }
  // restore the original extent
  inRegion1->SetAxisExtent(axis, inMin, inMax);
  inRegion2->SetAxisExtent(axis, inMin, inMax);
  outRegion->SetAxisExtent(axis, outMin, outMax);
}
  
  
//----------------------------------------------------------------------------
// Description:
// The execute method created by the subclass.
void vtkImageDyadicFilter::Execute(vtkImageRegion *inRegion1, 
				   vtkImageRegion *inRegion2, 
				   vtkImageRegion *outRegion)
{
  inRegion1 = inRegion2 = outRegion;
  vtkErrorMacro(<< "Subclass needs to suply an execute function.");
}

  



//============================================================================
// Stuff for filters that do not use the execute methods..
//============================================================================

//----------------------------------------------------------------------------
vtkImageRegion *vtkImageDyadicFilter::GetInput1Region(int dim, int *extent)
{
  int idx;
  int *imageExtent;
  vtkImageRegion *region;
  
  if ( ! this->Input1)
    {
    vtkErrorMacro(<< "Input1 is not set.");
    return NULL;
    }

  region = new vtkImageRegion;

  // This step is just error checking, and may be wastefull.  The Image
  // Information is automatically computed when UpdateRegion is called.
  this->Input1->UpdateImageInformation(region);
  region->SetAxes(this->GetAxes());
  imageExtent = region->GetImageExtent();
  for (idx = dim; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (imageExtent[idx*2] > 0 || imageExtent[idx*2 + 1] < 0)
      {
      vtkErrorMacro(<< "GetInputRegion1: dim = " << dim 
                    << ", unspecified dimensions do not include 0.");
      region->Delete();
      return NULL;
      }
    }
  
  // Note: This automatical sets the unspecified dimension extent to [0,0]
  region->SetExtent(dim, extent);
  this->Input1->UpdateRegion(region);
  
  return region;
}



//----------------------------------------------------------------------------
vtkImageRegion *vtkImageDyadicFilter::GetInput2Region(int dim, int *extent)
{
  int idx;
  int *imageExtent;
  vtkImageRegion *region;
  
  if ( ! this->Input2)
    {
    vtkErrorMacro(<< "Input2 is not set.");
    return NULL;
    }

  region = new vtkImageRegion;

  // This step is just error checking, and may be wastefull.  The Image
  // Information is automatically computed when UpdateRegion is called.
  this->Input2->UpdateImageInformation(region);
  region->SetAxes(this->GetAxes());
  imageExtent = region->GetImageExtent();
  for (idx = dim; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (imageExtent[idx*2] > 0 || imageExtent[idx*2 + 1] < 0)
      {
      vtkErrorMacro(<< "GetInputRegion2: dim = " << dim 
                    << ", unspecified dimensions do not include 0.");
      region->Delete();
      return NULL;
      }
    }
  
  // Note: This automatical sets the unspecified dimension extent to [0,0]
  region->SetExtent(dim, extent);
  this->Input2->UpdateRegion(region);
  
  return region;
}














