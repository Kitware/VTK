/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMultipleInputFilter.cxx
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
#include "vtkImageMultipleInputFilter.h"
#include "vtkImageCache.h"


//----------------------------------------------------------------------------
vtkImageMultipleInputFilter::vtkImageMultipleInputFilter()
{
  this->NumberOfInputs = 0;
  this->Inputs = NULL;
  this->Regions = NULL;
  this->UseExecuteMethodOn();
  this->InputMemoryLimit = 100000;   // 100 MBytes
}

//----------------------------------------------------------------------------
vtkImageMultipleInputFilter::~vtkImageMultipleInputFilter()
{
  if (this->Inputs)
    {
    delete [] this->Inputs;
    }
  if (this->Regions)
    {
    delete [] this->Regions;
    }
}

//----------------------------------------------------------------------------
void vtkImageMultipleInputFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkImageCachedSource::PrintSelf(os,indent);
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    os << indent << "Input " << idx << ": (" << this->Inputs[idx] << ")\n";
    }
}

// SGI had problems with new (vtkImageRegion *)[num];
typedef vtkImageRegion *vtkImageRegionPointer;
typedef vtkImageSource *vtkImageSourcePointer;
//----------------------------------------------------------------------------
// Called by constructor to set up input array.
void vtkImageMultipleInputFilter::SetNumberOfInputs(int num)
{
  int idx;
  
  // delete the previous arrays
  if (this->Inputs)
    {
    delete [] this->Inputs;
    }
  if (this->Regions)
    {
    delete [] this->Regions;
    }
  // Allocate new arrays.
  this->Inputs = new vtkImageSourcePointer[num];
  this->Regions = new vtkImageRegionPointer[num];
  // Initialize with NULLs.
  for (idx = 0; idx < num; ++idx)
    {
    this->Inputs[idx] = NULL;
    this->Regions[idx] = NULL;
    }
  
  this->NumberOfInputs = num;
  this->Modified();
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
unsigned long int vtkImageMultipleInputFilter::GetPipelineMTime()
{
  unsigned long int time1, time2;
  int idx;

  // This objects MTime
  // (Super class considers cache in case cache did not originate message)
  time1 = this->vtkImageCachedSource::GetPipelineMTime();
  // Look at input modified times.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if ( ! this->Inputs[idx])
      {
      vtkWarningMacro(<< "GetPipelineMTime: Input1 not set.");
      time2 = time1;
      }
    else
      {
      time2 = this->Inputs[idx]->GetPipelineMTime();
      }
    
    // Keep the larger of the two 
    if (time2 > time1)
      {
      time1 = time2;
      }
    }
  
  return time1;
}


//----------------------------------------------------------------------------
// Description:
// Set an Input of this filter. If a ScalarType has not been set,
// then the ScalarType of the input is used.
void vtkImageMultipleInputFilter::SetInput(int num, vtkImageSource *input)
{
  if (num < 0 || num >= this->NumberOfInputs)
    {
    vtkErrorMacro(<< "SetInput: " << num << ", cannot set input. " 
        << "Try setting NumberOfInputs first.");
    return;
    }
  
  // does this change anything?
  if (input == this->Inputs[num])
    {
    return;
    }
  
  this->Inputs[num] = input;
  this->Modified();

  // Should we use the data type from the input?
  this->CheckCache();      // make sure a cache exists
  if (this->Output->GetScalarType() == VTK_VOID)
    {
    this->Output->SetScalarType(input->GetScalarType());
    if (this->Output->GetScalarType() == VTK_VOID)
      {
      vtkWarningMacro(<< "SetInput1: Cannot determine ScalarType of input.");
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
void vtkImageMultipleInputFilter::UpdatePointData(int dim,
						  vtkImageRegion *outRegion)
{
  int idx;
  
  // If outBBox is empty return imediately.
  if (outRegion->IsEmpty())
    {
    return;
    }
    
  // Make sure the Inputs have been set.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if ( ! this->Inputs[idx])
      {
      vtkErrorMacro(<< "An input is not set.");
      return;
      }
    }
    
  // Determine whether to use the execute methods or the generate methods.
  if ( ! this->UseExecuteMethod)
    {
    this->vtkImageCachedSource::UpdatePointData(dim, outRegion);
    return;
    }
  
  // Make the input regions that will be used to generate the output region
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    this->Regions[idx] = new vtkImageRegion;
    // Fill in image information
    this->Inputs[idx]->UpdateImageInformation(this->Regions[idx]);
    // Translate to local coordinate system
    this->Regions[idx]->SetAxes(VTK_IMAGE_DIMENSIONS, this->Axes);
    // Set the default extents.
    this->Regions[idx]->SetExtent(VTK_IMAGE_DIMENSIONS, 
				    outRegion->GetExtent());
    }
  
  // Compute the required input region extent.
  // Copy to fill in extent of extra dimensions.
  this->ComputeRequiredInputRegionExtent(outRegion, this->Regions);

  
  // Use the input to fill the data of the region.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if ((this->Regions[idx]->GetVolume() / 1000) > this->InputMemoryLimit)
      {
      vtkWarningMacro(<< "Streaming not implemented yet.");
      }
    this->Inputs[idx]->UpdateRegion(this->Regions[idx]);
    }
  
  // fill the output region 
  this->Execute(dim, this->Regions, outRegion);

  // Save the new region in cache.
  this->Output->CacheRegion(outRegion);  
  
  // free the input regions
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    this->Regions[idx]->Delete();
    this->Regions[idx] = NULL;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method gets the boundary of the inputs then computes and returns 
// the boundary of the largest region that can be generated. 
void vtkImageMultipleInputFilter::UpdateImageInformation(
					 vtkImageRegion *outRegion)
{
  int idx;
  
  // Make sure the Input has been set.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if ( ! this->Inputs[idx])
      {
      vtkErrorMacro(<< "UpdateImageInformation: Input " << idx 
        << " is not set.");
      return;
      }
    }

  // Use the output for region 1
  this->Regions[0] = outRegion;
  this->Inputs[0]->UpdateImageInformation(outRegion);
  for (idx = 1; idx < this->NumberOfInputs; ++idx)
    {
    this->Regions[idx] = new vtkImageRegion;
    this->Inputs[idx]->UpdateImageInformation(this->Regions[idx]);
    }
  
  this->ComputeOutputImageInformation(this->Regions, outRegion);
  
  // Delete Regions
  this->Regions[0] = NULL;
  for (idx = 1; idx < this->NumberOfInputs; ++idx)
    {
    this->Regions[idx]->Delete();
    this->Regions[idx] = NULL;
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed an inRegion that holds the image information
// (image extent ...) of this filters input, and fills outRegion with
// the image information after this filter is finished.
// outImage is identical to inImage when this method is envoked, and
// outImage may be the same object as in image.
void vtkImageMultipleInputFilter::ComputeOutputImageInformation(
						vtkImageRegion **inRegions,
						vtkImageRegion *outRegion)
{
  // Default: Image information does not change (do nothing).
  // Avoid warnings
  inRegions = inRegions;
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
void vtkImageMultipleInputFilter::ComputeRequiredInputRegionExtent(
					       vtkImageRegion *outRegion,
					       vtkImageRegion **inRegions)
{
  int idx;
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    inRegions[idx]->SetExtent(VTK_IMAGE_DIMENSIONS, outRegion->GetExtent());
    }
}




//----------------------------------------------------------------------------
// Description:
// This execute method recursively loops over extra dimensions and
// calls the subclasses Execute method with lower dimensional regions.
void vtkImageMultipleInputFilter::Execute(int dim, vtkImageRegion **inRegions,
					  vtkImageRegion *outRegion)
{
  int coordinate, axis;
  int inMin, inMax;
  int outMin, outMax;
  int idx;

  // Terminate recursion?
  if (dim <= this->Dimensionality)
    {
    this->Execute(inRegions, outRegion);
    return;
    }
  
  // Get the extent of the forth dimension to be eliminated.
  axis = this->Axes[dim - 1];
  inRegions[0]->GetAxisExtent(axis, inMin, inMax);
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
    for (idx = 0; idx < this->NumberOfInputs; ++idx)
      {
      inRegions[idx]->SetAxisExtent(axis, coordinate, coordinate);
      }
    outRegion->SetAxisExtent(axis, coordinate, coordinate);
    this->Execute(dim - 1, inRegions, outRegion);
    }
  // restore the original extent
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    inRegions[idx]->SetAxisExtent(axis, inMin, inMax);
    }
  outRegion->SetAxisExtent(axis, outMax, outMax);
}
  
  
//----------------------------------------------------------------------------
// Description:
// The execute method created by the subclass.
void vtkImageMultipleInputFilter::Execute(vtkImageRegion **inRegions, 
					  vtkImageRegion *outRegion)
{
  inRegions = inRegions;
  outRegion = outRegion;
  vtkErrorMacro(<< "Subclass needs to suply an execute function.");
}

  













