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
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageMultipleInputFilter.h"


//----------------------------------------------------------------------------
vtkImageMultipleInputFilter::vtkImageMultipleInputFilter()
{
  this->NumberOfInputs = 0;
  this->Inputs = NULL;
  this->Regions = NULL;
  this->Bypass = 0;
  this->Updating = 0;
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
  
  os << indent << "FilteredAxes: ";
  if (this->NumberOfFilteredAxes == 0)
    {
    os << indent << "None\n";
    }
  else
    {
    os << indent << "(" << vtkImageAxisNameMacro(this->FilteredAxes[0]);
    for (idx = 1; idx < this->NumberOfFilteredAxes; ++idx)
      {
      os << ", " << vtkImageAxisNameMacro(this->FilteredAxes[idx]);
      }
    os << ")\n";
    }
  
  os << indent << "Bypass: " << this->Bypass << "\n";
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    os << indent << "Input " << idx << ": (" << this->Inputs[idx] << ")\n";
    }

  vtkImageSource::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkImageMultipleInputFilter::SetFilteredAxes(int num, int *axes)
{
  int idx;
  int modified = 0;
  
  if (num > 4)
    {
    vtkWarningMacro("SetFilteredAxes: Too many axes");
    num = 4;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    if (this->FilteredAxes[idx] != axes[idx])
      {
      modified = 1;
      this->FilteredAxes[idx] = axes[idx];
      }
    }
  if (num != this->NumberOfFilteredAxes)
    {
    modified = 1;
    this->NumberOfFilteredAxes = num;
    }
  
  if (modified)
    {
    this->Modified();
    this->SetExecutionAxes(num, this->FilteredAxes);
    }
}


// SGI had problems with new (vtkImageRegion *)[num];
typedef vtkImageRegion *vtkImageRegionPointer;
typedef vtkImageCache *vtkImageCachePointer;
//----------------------------------------------------------------------------
// Called by constructor to set up input array.
void vtkImageMultipleInputFilter::SetNumberOfInputs(int num)
{
  int idx;
  vtkImageCachePointer *inputs;
  vtkImageRegionPointer *regions;

  // in case nothing has changed.
  if (num == this->NumberOfInputs)
    {
    return;
    }
  
  // Allocate new arrays.
  inputs = new vtkImageCachePointer[num];
  regions = new vtkImageRegionPointer[num];

  // Initialize with NULLs.
  for (idx = 0; idx < num; ++idx)
    {
    inputs[idx] = NULL;
    regions[idx] = NULL;
    }

  // Copy old inputs
  for (idx = 0; idx < num && idx < this->NumberOfInputs; ++idx)
    {
    inputs[idx] = this->Inputs[idx];
    }
  
  // delete the previous arrays
  if (this->Inputs)
    {
    delete [] this->Inputs;
    }
  if (this->Regions)
    {
    delete [] this->Regions;
    }
  
  // Set the new arrays
  this->Inputs = inputs;
  this->Regions = regions;
  
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
  time1 = this->vtkImageSource::GetPipelineMTime();
  // Look at input modified times.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if ( ! this->Inputs[idx])
      {
      vtkWarningMacro(<< "GetPipelineMTime: Input " << idx << " not set.");
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
// Adds an input to the first null position in the input list.
// Expands the list memory if necessary
void vtkImageMultipleInputFilter::AddInput(vtkImageCache *input)
{
  int idx;
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] == NULL)
      {
      this->Inputs[idx] = input;
      return;
      }
    }
  
  this->SetNumberOfInputs(this->NumberOfInputs + 1);
  this->Inputs[this->NumberOfInputs - 1] = input;
}

//----------------------------------------------------------------------------
// Description:
// Set an Input of this filter. If a ScalarType has not been set,
// then the ScalarType of the input is used.
void vtkImageMultipleInputFilter::SetInput(int num, vtkImageCache *input)
{
  if (num < 0)
    {
    vtkErrorMacro(<< "SetInput: " << num << ", cannot set input. ");
    return;
    }
  // Expand array if necessary.
  if (num >= this->NumberOfInputs)
    {
    this->SetNumberOfInputs(num + 1);
    }
  
  // does this change anything?
  if (input == this->Inputs[num])
    {
    return;
    }
  
  this->Inputs[num] = input;
  this->Modified();
}




//----------------------------------------------------------------------------
// Description:
// Called by cache
void vtkImageMultipleInputFilter::Update()
{
  vtkImageRegion *outRegion;
  int idx, idx2;
  
  // We could handle NULLs in our input list, but ...
  if ( ! this->Inputs || ! this->Inputs[0])
    {
    vtkErrorMacro("Input0 required");
    return;
    }
  
  // prevent infinite update loops.
  if (this->Updating)
    {
    return;
    }
  this->Updating = 1;
  
  // Make sure there is an output.
  this->CheckCache();
  
  // In case this update is called directly.
  this->UpdateImageInformation();
  this->Output->ClipUpdateExtentWithWholeExtent();
  
  // Handle bypass condition.
  if (this->Bypass)
    {
    this->Inputs[0]->SetUpdateExtent(this->Output->GetUpdateExtent());
    this->Inputs[0]->Update();
    this->Output->SetScalarData(this->Inputs[0]->GetScalarData());
    this->Output->SetNumberOfScalarComponents(
			      this->Inputs[0]->GetNumberOfScalarComponents());
    // release input data
    if (this->Inputs[0]->ShouldIReleaseData())
      {
      this->Inputs[0]->ReleaseData();
      }
    this->Updating = 0;
    return;
    }  
  
  // Make sure the subclss has defined the execute dimensionality
  // It is needed to terminate recursion.
  if (this->NumberOfExecutionAxes < 0)
    {
    vtkErrorMacro(<< "Subclass has not set NumberOfExecutionAxes");
    this->Updating = 0;
    return;
    }

  // Get the output region.
  // Note: outRegion does not allocate until first "GetScalarPointer" call
  outRegion = this->Output->GetScalarRegion();
  outRegion->SetAxes(5, this->ExecutionAxes);
  
  // If outBBox is empty return imediately.
  if (outRegion->IsEmpty())
    {
    outRegion->Delete();
    this->Updating = 0;
    return;
    }
    
  // Make sure the Inputs have been set.
  for (idx = 1; idx < this->NumberOfInputs; ++idx)
    {
    if ( ! this->Inputs[idx])
      {
      vtkErrorMacro(<< "An input is not set.");
      this->Updating = 0;
      return;
      }
    }
    
  // Compute the required input region extents.
  // Copy to fill in extent of extra dimensions.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    this->Inputs[idx]->SetUpdateExtent(this->Output->GetUpdateExtent());
    }
  this->ComputeRequiredInputUpdateExtent(this->Output, this->Inputs);

  // ... no streaming implemented yet ...
  
  // Get the input regions
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
      this->Inputs[idx]->Update();
      this->Regions[idx] = this->Inputs[idx]->GetScalarRegion();
      this->Regions[idx]->SetAxes(5, this->ExecutionAxes);
      // Make sure we got the input.
      if ( ! this->Regions[idx]->AreScalarsAllocated())
	{
	vtkErrorMacro("Update: Could not get input " << idx);
	for (idx2 = 0; idx2 <= idx; ++idx2)
	  {
	  this->Regions[idx2]->Delete();
	  this->Regions[idx2] = NULL;
	  }
	this->Updating = 0;
	return;
	}   
    }

  // The StartMethod call is placed here to be after updating the input.
  if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
  // fill the output region 
  this->RecursiveLoopExecute(VTK_IMAGE_DIMENSIONS, this->Regions, outRegion);
  if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
  
  // inRegion is just a handle to the data.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    this->Regions[idx]->Delete();
    this->Regions[idx] = NULL;
  
    // Like the graphics pipeline this source releases inputs data.
    if (this->Inputs[idx]->ShouldIReleaseData())
      {
      this->Inputs[idx]->ReleaseData();
      }
    }
  
  // Delete the container for the data (not the data).
  outRegion->Delete();
  
  this->Updating = 0;
}

//----------------------------------------------------------------------------
// Description:
// This method gets the boundary of the inputs then computes and returns 
// the boundary of the largest region that can be generated. 
void vtkImageMultipleInputFilter::UpdateImageInformation()
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
    this->Inputs[idx]->UpdateImageInformation();
    }
  // make sure we have an output
  this->CheckCache();
  
  // Set the defaults from input1
  this->Output->SetWholeExtent(this->Inputs[0]->GetWholeExtent());
  this->Output->SetSpacing(this->Inputs[0]->GetSpacing());
  this->Output->SetOrigin(this->Inputs[0]->GetOrigin());
  if ( ! this->Bypass)
    {
    // Let the subclass modify the default.
    this->ExecuteImageInformation(this->Inputs, this->Output);
    }
  
  
  // If the ScalarType of the output has not been set yet,
  // set it to be the same as input.
  if (this->Output->GetScalarType() == VTK_VOID)
    {
    this->Output->SetScalarType(this->Inputs[0]->GetScalarType());
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed an inRegion that holds the image information
// (image extent ...) of this filters input, and fills outRegion with
// the image information after this filter is finished.
// outImage is identical to inImage when this method is envoked, and
// outImage may be the same object as in image.
void vtkImageMultipleInputFilter::ExecuteImageInformation(vtkImageCache **ins,
							  vtkImageCache *out)
{
  // Default: Image information does not change (do nothing).
  // Avoid warnings
  ins = ins;
  out = out;
}



//----------------------------------------------------------------------------
// Description:
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// extent of the output region.  After this method finishes, "region" should 
// have the extent of the required input region.  The default method assumes
// the required input extent are the same as the output extent.
// Note: The splitting methods call this method with outRegion = inRegion.
void vtkImageMultipleInputFilter::ComputeRequiredInputUpdateExtent(
					       vtkImageCache *out,
					       vtkImageCache **ins)
{
  int idx;
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    ins[idx]->SetUpdateExtent(out->GetUpdateExtent());
    }
}



//----------------------------------------------------------------------------
// Description:
// This execute method recursively loops over extra dimensions and
// calls the subclasses Execute method with lower dimensional regions.
void vtkImageMultipleInputFilter::RecursiveLoopExecute(int dim, 
		       vtkImageRegion **inRegions, vtkImageRegion *outRegion)
{
  // Terminate recursion?
  if (dim <= this->NumberOfExecutionAxes)
    {
    this->Execute(inRegions, outRegion);
    return;
    }
  else
    {
    int coordinate, axis;
    int inMin, inMax;
    int outMin, outMax;
    int idx;
    
    // Get the extent of the forth dimension to be eliminated.
    axis = this->ExecutionAxes[dim - 1];
    inRegions[0]->GetAxisExtent(axis, inMin, inMax);
    outRegion->GetAxisExtent(axis, outMin, outMax);
    
    // Extra axis of in and out must have the same extent
    // NOTE: Only Checking the first input...
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
      this->RecursiveLoopExecute(dim - 1, inRegions, outRegion);
      }
    // restore the original extent
    for (idx = 0; idx < this->NumberOfInputs; ++idx)
      {
      inRegions[idx]->SetAxisExtent(axis, inMin, inMax);
      }
    outRegion->SetAxisExtent(axis, outMax, outMax);
    }
}

  
  
//----------------------------------------------------------------------------
// Description:
// The execute method created by the subclass.
void vtkImageMultipleInputFilter::Execute(vtkImageRegion **inRegions, 
					  vtkImageRegion *outRegion)
{
  inRegions = inRegions;
  outRegion = outRegion;
  vtkErrorMacro(<< "Subclass needs to supply an execute function.");
}

  













