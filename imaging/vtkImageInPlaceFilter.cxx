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
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageInPlaceFilter.h"


//----------------------------------------------------------------------------
vtkImageInPlaceFilter::vtkImageInPlaceFilter()
{
  this->Input = NULL;
}

//----------------------------------------------------------------------------
void vtkImageInPlaceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageCachedSource::PrintSelf(os,indent);
  os << indent << "Input: (" << this->Input << ").\n";
}

//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline upto and including this filter
// Note: current implementation may create a cascade of GetPipelineMTime calls.
// Each GetPipelineMTime call propagates the call all the way to the original
// source.  
unsigned long int vtkImageInPlaceFilter::GetPipelineMTime()
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
void vtkImageInPlaceFilter::SetInput(vtkImageCache *input)
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
// This method is called by the cache.
void vtkImageInPlaceFilter::Update(vtkImageRegion *outRegion)
{
  vtkImageRegion *inRegion;

  // Make sure the subclss has defined the execute dimensionality
  // It is needed to terminate recursion.
  if (this->ExecuteDimensionality < 0)
    {
    vtkErrorMacro(<< "Subclass has not set ExecuteDimensionality");
    return;
    }

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
  
  // Make the input region that will be used to generate the output region
  inRegion = vtkImageRegion::New();
  // Fill in image information (ComputeRequiredInputExtent may need it)
  this->Input->UpdateImageInformation(inRegion);
  // Set the coordinate system
  inRegion->SetAxes(VTK_IMAGE_DIMENSIONS, this->Axes);
  
  // Compute the required input region extent.
  // Copy to fill in extent of extra dimensions.
  inRegion->SetExtent(VTK_IMAGE_DIMENSIONS, outRegion->GetExtent());
  this->ComputeRequiredInputRegionExtent(outRegion, inRegion);

  // No streaming for inplace filters

  // Use the input to fill the data of the region.
  this->Input->Update(inRegion);
  // Make sure we got the input.
  if ( ! inRegion->AreScalarsAllocated())
    {
    vtkErrorMacro("Update: Could not get input");
    return;
    }
  
  // Copy  Scalars (by reference if possible) from input to output.
  if ((inRegion->GetData()->GetReferenceCount() > 1) ||
      (inRegion->GetData()->GetScalars()->GetReferenceCount() > 1) ||
      (inRegion->GetScalarType() != outRegion->GetScalarType()))
    {
    // we have to copy the data
    vtkDebugMacro(<< "Update: Cannot copy by reference.");
    outRegion->CopyRegionData(inRegion);
    }
  else
    {
    // We can just reference the scalars
    vtkDebugMacro(<< "Update: Copying scalars by reference.");
    outRegion->GetData()->SetScalars(inRegion->GetData()->GetScalars());
    }

  // fill the output region 
  // The inRegion is passed just in case.
  if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
  this->RecursiveLoopExecute(VTK_IMAGE_DIMENSIONS, inRegion, outRegion);
  if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);

  // free the input region
  inRegion->Delete();

  // Save the new region in cache.
  // I am not happy with the necessity of the call CacheRegion.
  // Getting the region from the cache originally seems more consistent
  // and more compatable with the visualization pipeline.
  this->Output->CacheRegion(outRegion);
}


//----------------------------------------------------------------------------
// Description:
// This method gets the boundary of the input then computes and returns 
// the boundary of the largest region that can be generated. 
void vtkImageInPlaceFilter::UpdateImageInformation(vtkImageRegion *region)
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
void vtkImageInPlaceFilter::ComputeOutputImageInformation(vtkImageRegion *inRegion,
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
vtkImageInPlaceFilter::ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
						 vtkImageRegion *inRegion)
{
  inRegion->SetExtent(outRegion->GetExtent());
}



//----------------------------------------------------------------------------
// Description:
// This execute method recursively loops over extra dimensions and
// calls the subclasses Execute method with lower dimensional regions.
void
vtkImageInPlaceFilter::RecursiveLoopExecute(int dim, vtkImageRegion *inRegion, 
					    vtkImageRegion *outRegion)
{
  // Terminate recursion?
  if (dim <= this->ExecuteDimensionality)
    {
    this->Execute(inRegion, outRegion);
    return;
    }
  else
    {
    int coordinate, axis;
    int inMin, inMax;
    int outMin, outMax;
    
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
      this->RecursiveLoopExecute(dim - 1, inRegion, outRegion);
      }
    // restore the original extent
    inRegion->SetAxisExtent(axis, inMin, inMax);
    outRegion->SetAxisExtent(axis, outMin, outMax);
    }
}




//----------------------------------------------------------------------------
// Description:
// The execute method created by the subclass.
void vtkImageInPlaceFilter::Execute(vtkImageRegion *inRegion, 
				    vtkImageRegion *outRegion)
{
  inRegion = outRegion;
  vtkErrorMacro(<< "Subclass needs to supply an execute function.");
}

  










