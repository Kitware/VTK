/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTwoOutputFilter.cxx
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
#include "vtkImageTwoOutputFilter.h"
#include "vtkImageSimpleCache.h"

//----------------------------------------------------------------------------
vtkImageTwoOutputFilter::vtkImageTwoOutputFilter()
{
  this->Input = NULL;
  this->Output1 = NULL;
  // invalid settings
  this->ExecuteDimensionality = -1;
}

//----------------------------------------------------------------------------
void vtkImageTwoOutputFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageCachedSource::PrintSelf(os,indent);
  os << indent << "Output1: (" << this->Output1 << ").\n";
  os << indent << "Input: (" << this->Input << ").\n";
}

//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline upto and including this filter
// Note: current implementation may create a cascade of GetPipelineMTime calls.
// Each GetPipelineMTime call propagates the call all the way to the original
// source.  
unsigned long int vtkImageTwoOutputFilter::GetPipelineMTime()
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
void vtkImageTwoOutputFilter::SetInput(vtkImageCache *input)
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
  this->CheckCache1();      // make sure a cache exists
  if (this->Output->GetScalarType() == VTK_VOID)
    {
    this->Output->SetScalarType(input->GetScalarType());
    if (this->Output->GetScalarType() == VTK_VOID)
      {
      vtkErrorMacro(<< "SetInput: Cannot determine ScalarType of input.");
      }
    }
  if (this->Output1->GetScalarType() == VTK_VOID)
    {
    this->Output1->SetScalarType(input->GetScalarType());
    if (this->Output1->GetScalarType() == VTK_VOID)
      {
      vtkErrorMacro(<< "SetInput: Cannot determine ScalarType of input.");
      }
    }
}


//----------------------------------------------------------------------------
// Description:
// This method is called by the cache.  
void vtkImageTwoOutputFilter::Update(vtkImageRegion *outRegion)
{
  vtkImageRegion *inRegion;
  vtkImageRegion *outRegion0;
  vtkImageRegion *outRegion1;

  // Make sure the subclss has defined the execute dimensionality.
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
  // What extent do we need to request from the input.
  this->ComputeRequiredInputRegionExtent(outRegion, inRegion);

  // Get the input data
  this->Input->Update(inRegion);
  if ( ! inRegion->AreScalarsAllocated())
    {
    vtkErrorMacro("Update(region): Cannot get input");
    return;
    }

  // Make the two output regions
  outRegion0 = vtkImageRegion::New();
  outRegion1 = vtkImageRegion::New();
  // Copy every thing but the data (there is no data)
  // The two regions do have to have the same extent,
  // (because we do not know which caches is making the original request).
  outRegion0->SetScalarType(outRegion->GetScalarType());
  outRegion1->SetScalarType(outRegion->GetScalarType());
  outRegion0->SetAxes(outRegion->GetAxes());
  outRegion1->SetAxes(outRegion->GetAxes());
  outRegion0->SetExtent(outRegion->GetExtent());
  outRegion1->SetExtent(outRegion->GetExtent());

  // Pass on to a method that can be called recursively (for streaming)
  // inRegion is passed to avoid setting up a new region many times.
  if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
  this->RecursiveLoopExecute(VTK_IMAGE_DIMENSIONS, 
			     inRegion, outRegion0, outRegion1);
  if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
  
  // free the input region
  inRegion->Delete();

  // Make sure both caches exist
  this->CheckCache();
  this->CheckCache1();  
  
  // Save the new region in cache.
  // This is the place where "CacheRegion" is important.  It could
  // be done in another way.  Regions could be retrieved from the cache.
  this->Output->CacheRegion(outRegion0);
  this->Output1->CacheRegion(outRegion1);
  
  // Free the output regions
  outRegion0->Delete();
  outRegion1->Delete();  
}



  
//----------------------------------------------------------------------------
// Description:
// This execute method recursively loops over extra dimensions and
// calls the subclasses Execute method with lower dimensional regions.
void vtkImageTwoOutputFilter::RecursiveLoopExecute(int dim, 
						   vtkImageRegion *inRegion, 
						   vtkImageRegion *outRegion0,
						   vtkImageRegion *outRegion1)
{
  // Terminate recursion?
  if (dim <= this->ExecuteDimensionality)
    {
    this->Execute(inRegion, outRegion0, outRegion1);
    return;
    }
  else
    {
    int coordinate, axis;
    int inMin, inMax;
    int outMin0, outMax0;
    int outMin1, outMax1;
    
    // Get the extent of the dimension to be eliminated.
    axis = this->Axes[dim - 1];
    inRegion->GetAxisExtent(axis, inMin, inMax);
    outRegion0->GetAxisExtent(axis, outMin0, outMax0);
    outRegion1->GetAxisExtent(axis, outMin1, outMax1);

    // The axis should have the same extent.
    if (inMin!=outMin0 || inMax!=outMax0 || inMin!=outMin1 || inMax!=outMax1) 
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
      outRegion0->SetAxisExtent(axis, coordinate, coordinate);
      outRegion1->SetAxisExtent(axis, coordinate, coordinate);
      this->RecursiveLoopExecute(dim - 1, inRegion, 
				 outRegion0, outRegion1);
      }
    // restore the original extent
    inRegion->SetAxisExtent(axis, inMin, inMax);
    outRegion0->SetAxisExtent(axis, outMin0, outMax0);
    outRegion1->SetAxisExtent(axis, outMin1, outMax1);
    }
}










//----------------------------------------------------------------------------
// Description:
// This method gets the boundary of the input then computes and returns 
// the boundary of the largest region that can be generated. 
void vtkImageTwoOutputFilter::UpdateImageInformation(vtkImageRegion *region)
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
// Given a region with input image info, compute output image info.
// Since image info should be the same for both outputs, 
// I am leaving this function with only one output region.
void vtkImageTwoOutputFilter::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
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
// Since both out ragios have to have the same extent, there is only one
// out region.
void vtkImageTwoOutputFilter::ComputeRequiredInputRegionExtent(
		       vtkImageRegion *outRegion, vtkImageRegion *inRegion)
{
  inRegion->SetExtent(outRegion->GetExtent());
}



//----------------------------------------------------------------------------
// Description:
// The execute method created by the subclass.
void vtkImageTwoOutputFilter::Execute(vtkImageRegion *inRegion, 
				      vtkImageRegion *outRegion0,
				      vtkImageRegion *outRegion1)
{
  inRegion = outRegion0 = outRegion1;
  vtkErrorMacro(<< "Subclass needs to suply an execute function.");
}



//----------------------------------------------------------------------------
// Description:
// Returns the cache object of the source.  If one does not exist, a default
// is created.
vtkImageCache *vtkImageTwoOutputFilter::GetOutput1()
{
  this->CheckCache1();
  
  return this->Output1;
}


//----------------------------------------------------------------------------
// Description:
// This private method creates a cache if one has not been set.
// ReleaseDataFlag is turned on.
void vtkImageTwoOutputFilter::CheckCache1()
{
  // create a default cache if one has not been set
  if ( ! this->Output1)
    {
    this->Output1 = vtkImageSimpleCache::New();
    this->Output1->ReleaseDataFlagOn();
    this->Output1->SetSource(this);
    this->Modified();
    }
}



//----------------------------------------------------------------------------
// Description:
// This method sets the values of the caches ReleaseDataFlag.  When this flag
// is set, the caches release their data after every generate.  When a default
// cache is created, this flag is automatically set.
void vtkImageTwoOutputFilter::SetReleaseDataFlag(int value)
{
  this->CheckCache();
  this->CheckCache1();
  this->Output->SetReleaseDataFlag(value);
  this->Output1->SetReleaseDataFlag(value);
}







