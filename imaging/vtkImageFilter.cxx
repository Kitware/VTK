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
#include "vtkTimerLog.h"
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageFilter.h"

//----------------------------------------------------------------------------
vtkImageFilter::vtkImageFilter()
{
  this->Input = NULL;
  this->SetSplitOrder(4,3,2,1,0);
  this->InputMemoryLimit = 5000000;   // 5 GB
  // invalid settings
  this->ExecuteDimensionality = -1;
}

//----------------------------------------------------------------------------
void vtkImageFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkImageCachedSource::PrintSelf(os,indent);
  os << indent << "Input: (" << this->Input << ").\n";
  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
  os << indent << "SplitOrder: ("<< vtkImageAxisNameMacro(this->SplitOrder[0]);
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << vtkImageAxisNameMacro(this->SplitOrder[idx]);
    }
  os << ")\n";
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
void vtkImageFilter::SetInput(vtkImageCache *input)
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
// This method is called by the cache.  It eventually calls the
// Execute(vtkImageRegion *, vtkImageRegion *) method.
// ImageInformation has already been updated by this point, 
// and outRegion is in local coordinates.
// This method will stream to get the input, and loops over extra axes.
void vtkImageFilter::Update(vtkImageRegion *outRegion)
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
  
  // Pass on to a method that can be called recursively (for streaming)
  // inRegion is passed to avoid setting up a new region many times.
  this->RecursiveStreamUpdate(inRegion, outRegion);

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
// This method can be called recursively for streaming.
// The extent of the outRegion changes, dim remains the same.
void vtkImageFilter::RecursiveStreamUpdate(vtkImageRegion *inRegion,
					   vtkImageRegion *outRegion)
{
  int memory;

  //vtkTimerLogMacro("Entering Update");

  // Compute the required input region extent.
  // Copy to fill in extent of extra dimensions.
  inRegion->SetExtent(VTK_IMAGE_DIMENSIONS, outRegion->GetExtent());
  this->ComputeRequiredInputRegionExtent(outRegion, inRegion);
  
  // determine the amount of memory that will be used by the input region.
  memory = inRegion->GetExtentMemorySize();
  
  // Split the outRegion if we are streaming.
  if (memory > this->InputMemoryLimit)
    {
    int splitAxisIdx, splitAxis;
    int min, max, mid;
    // We need to split the inRegion.
    // Pick an axis to split
    splitAxisIdx = 0;
    splitAxis = this->SplitOrder[splitAxisIdx];
    outRegion->GetAxisExtent(splitAxis, min, max);
    while ( (min == max) && splitAxisIdx < this->NumberOfSplitAxes)
      {
      ++splitAxisIdx;
      splitAxis = this->SplitOrder[splitAxisIdx];
      outRegion->GetAxisExtent(splitAxis, min, max);
      }
    // Make sure we can actually split the axis
    if (min < max)
      {
      // Set the first half to update
      mid = (min + max) / 2;
      vtkDebugMacro(<< "RecursiveStreamUpdate: Splitting " 
        << vtkImageAxisNameMacro(splitAxis) << ": memory = " << memory <<
        ", extent = " << min << "->" << mid << " | " << mid+1 << "->" << max);
      outRegion->SetAxisExtent(splitAxis, min, mid);
      this->RecursiveStreamUpdate(inRegion, outRegion);
      // Set the second half to update
      outRegion->SetAxisExtent(splitAxis, mid+1, max);
      this->RecursiveStreamUpdate(inRegion, outRegion);
      // Restore the original extent
      outRegion->SetAxisExtent(splitAxis, min, max);
      return;
      }
    else
      {
      // Cannot split any more.  Ignore memory limit and continue.
      vtkWarningMacro(<< "UpdatePointData2: Cannot split. memory = "
        << memory << ", limit = " << this->InputMemoryLimit << ", "
        << vtkImageAxisNameMacro(splitAxis) << ": " << min << "->" << max);
      }
    }

  // No Streaming required.
  // Use the input to fill the data of the region.
  this->Input->Update(inRegion);
  // Make sure we got the input.
  if ( ! inRegion->AreScalarsAllocated())
    {
    vtkErrorMacro("RecursiveStreamUpdate: Could not get input");
    return;
    }  
  
  // The StartMethod call is placed here to be after updating the input.
  if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
  // fill the output region 
  this->RecursiveLoopExecute(VTK_IMAGE_DIMENSIONS, inRegion, outRegion);
  if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
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
// ExecuteDimensionality is used to terminate the recursion.
void vtkImageFilter::RecursiveLoopExecute(int dim, vtkImageRegion *inRegion, 
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
void vtkImageFilter::Execute(vtkImageRegion *inRegion, 
			     vtkImageRegion *outRegion)
{
  inRegion = outRegion;
  vtkErrorMacro(<< "Subclass needs to supply an execute function.");
}



//----------------------------------------------------------------------------
void vtkImageFilter::SetSplitOrder(int num, int *axes)
{
  int idx;
  
  // Error checking
  if (num < 0 || num > VTK_IMAGE_DIMENSIONS)
    {
    vtkErrorMacro(<< "SetSplitOrder: Bad num " << num);
    return;
    }

  for (idx = 0; idx < num; ++idx)
    {
    this->SplitOrder[idx] = axes[idx];
    }
  this->NumberOfSplitAxes = num;
}
//----------------------------------------------------------------------------
void vtkImageFilter::GetSplitOrder(int num, int *axes)
{
  int idx;
  
  // Error checking
  if (num < 0 || num > this->NumberOfSplitAxes)
    {
    vtkErrorMacro(<< "GetSplitOrder: Bad num " << num);
    return;
    }

  for (idx = 0; idx < num; ++idx)
    {
    axes[idx] = this->SplitOrder[idx];
    }
}

  
    











