/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTwoInputFilter.cxx
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
#include "vtkImageTwoInputFilter.h"


//----------------------------------------------------------------------------
vtkImageTwoInputFilter::vtkImageTwoInputFilter()
{
  this->FilteredAxes[0] = VTK_IMAGE_X_AXIS;
  this->FilteredAxes[1] = VTK_IMAGE_Y_AXIS;
  this->FilteredAxes[2] = VTK_IMAGE_Z_AXIS;
  this->FilteredAxes[3] = VTK_IMAGE_TIME_AXIS;
  this->NumberOfFilteredAxes = 2;
  this->Input1 = NULL;
  this->Input2 = NULL;
  this->Bypass = 0;
  this->Updating = 0;
  
  // Invalid
  this->NumberOfExecutionAxes = -1;
}

//----------------------------------------------------------------------------
void vtkImageTwoInputFilter::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "Input1: (" << this->Input1 << ")\n";
  os << indent << "Input2: (" << this->Input2 << ")\n";

  vtkImageSource::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkImageTwoInputFilter::SetFilteredAxes(int num, int *axes)
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
    }
  this->SetExecutionAxes(num, this->FilteredAxes);
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
unsigned long int vtkImageTwoInputFilter::GetPipelineMTime()
{
  unsigned long int time1, time2;

  // This objects MTime
  // (Super class considers cache in case cache did not originate message)
  time1 = this->vtkImageSource::GetPipelineMTime();
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
void vtkImageTwoInputFilter::SetInput1(vtkImageCache *input)
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
}



//----------------------------------------------------------------------------
// Description:
// Set the Input2 of this filter. If a ScalarType has not been set,
// then the ScalarType of the input is used.
void vtkImageTwoInputFilter::SetInput2(vtkImageCache *input)
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
}



//----------------------------------------------------------------------------
// Description:
// This method is usually called by the cache.  It loops over axes that
// are not in the ExecutionAxes list. 
void vtkImageTwoInputFilter::Update()
{
  vtkImageRegion *inRegion1, *inRegion2, *outRegion;
  
  // Make sure the Input has been set.
  if ( ! this->Input1)
    {
    vtkErrorMacro(<< "Input1 is not set.");
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
    this->Input1->SetUpdateExtent(this->Output->GetUpdateExtent());
    this->Input1->Update();
    this->Output->SetScalarData(this->Input1->GetScalarData());
    this->Output->SetNumberOfScalarComponents(
		      this->Input1->GetNumberOfScalarComponents());
    // release input data
    if (this->Input1->ShouldIReleaseData())
      {
      this->Input1->ReleaseData();
      }
    this->Updating = 0;    
    return;
    }  
  
  // Make sure the subclass has defined the execute dimensionality
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
    
  // Compute the required input region extents.
  // Copy to fill in extent of extra dimensions.
  this->Input1->SetUpdateExtent(this->Output->GetUpdateExtent());
  if (this->Input2)
    {
    this->Input2->SetUpdateExtent(this->Output->GetUpdateExtent());
    }
  this->ComputeRequiredInputUpdateExtent(this->Output, this->Input1, 
					 this->Input2);

  // ... no streaming implemented yet ...

  // Get the input regions
  this->Input1->Update();
  inRegion1 = this->Input1->GetScalarRegion();
  inRegion1->SetAxes(5, this->ExecutionAxes);
  if ( ! inRegion1->AreScalarsAllocated())
    {
    vtkErrorMacro("Update: Could not get input1");
    inRegion1->Delete();
    outRegion->Delete();
    this->Updating = 0;
    return;
    }   

  if (this->Input2)
    {
    this->Input2->Update();
    inRegion2 = this->Input2->GetScalarRegion();
    inRegion2->SetAxes(5, this->ExecutionAxes);
    // Make sure we got the input2.
    if ( ! inRegion2->AreScalarsAllocated())
      {
      vtkErrorMacro("Update: Could not get input2");
      inRegion1->Delete();
      inRegion2->Delete();
      outRegion->Delete();
      this->Updating = 0;
      return;
      }   
    }
  else
    {
    inRegion2 = NULL;
    }

  // The StartMethod call is placed here to be after updating the input.
  if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
  // fill the output region 
  this->RecursiveLoopExecute(VTK_IMAGE_DIMENSIONS, inRegion1, inRegion2, 
			     outRegion);
  if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
  
  // inRegion is just a handle to the data.
  inRegion1->Delete();
  if (inRegion2)
    {
    inRegion2->Delete();
    }
  
  // Like the graphics pipeline this source releases inputs data.
  if (this->Input1->ShouldIReleaseData())
    {
    this->Input1->ReleaseData();
    }
  if (this->Input2 && this->Input2->ShouldIReleaseData())
    {
    this->Input2->ReleaseData();
    }
  
  // Delete the container for the data (not the data).
  outRegion->Delete();
  
  this->Updating = 0;
}


//----------------------------------------------------------------------------
// Description:
// This method sets the WholeExtent, Spacing and Origin of the output.
void vtkImageTwoInputFilter::UpdateImageInformation()
{
  // Make sure the Input has been set.
  if ( ! this->Input1)
    {
    vtkErrorMacro(<< "UpdateImageInformation: Input1 is not set.");
    return;
    }
  // make sure we have an output
  this->CheckCache();
  
  this->Input1->UpdateImageInformation();
  if (this->Input2)
    {
    this->Input2->UpdateImageInformation();
    }
  
  // Set the defaults from input1
  this->Output->SetWholeExtent(this->Input1->GetWholeExtent());
  this->Output->SetSpacing(this->Input1->GetSpacing());
  this->Output->SetOrigin(this->Input1->GetOrigin());
  if ( ! this->Bypass)
    {
    // Let the subclass modify the default.
    this->ExecuteImageInformation(this->Input1, this->Input2, this->Output);
    }
  
  // If the ScalarType of the output has not been set yet,
  // set it to be the same as input.
  if (this->Output->GetScalarType() == VTK_VOID)
    {
    this->Output->SetScalarType(this->Input1->GetScalarType());
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed an inRegion that holds the image information
// (image extent ...) of this filters input, and fills outRegion with
// the image information after this filter is finished.
// outImage is identical to inImage when this method is envoked, and
// outImage may be the same object as in image.
void vtkImageTwoInputFilter::ExecuteImageInformation(vtkImageCache *in1,
						     vtkImageCache *in2,
						     vtkImageCache *out)

{
  // Default: Image information does not change (do nothing).
  // Avoid warnings
  in1 = in1;
  in2 = in2;
  out = out;
}



//----------------------------------------------------------------------------
// Description:
// This method can be overriden in a subclass to compute the inputs
// UpdateExtents needed to generate the output UpdateExtent.
// By default the input is set to the same as the output before this
// method is called.
void vtkImageTwoInputFilter::ComputeRequiredInputUpdateExtent(
					      vtkImageCache *out,
					      vtkImageCache *in1,
					      vtkImageCache *in2)
{
  in1 = in1;
  in2 = in2;
}



//----------------------------------------------------------------------------
// Description:
// This execute method recursively loops over extra dimensions and
// calls the subclasses Execute method with lower dimensional regions.
// NumberOfExecutionAxes is used to terminate the recursion.
void vtkImageTwoInputFilter::RecursiveLoopExecute(int dim, 
						  vtkImageRegion *inRegion1,
						  vtkImageRegion *inRegion2,
						  vtkImageRegion *outRegion)
{
  // Terminate recursion?
  if (dim <= this->NumberOfExecutionAxes)
    {
    this->Execute(inRegion1, inRegion2, outRegion);
    return;
    }
  else
    {
    int coordinate, axis;
    int inMin1, inMax1, inMin2, inMax2;
    int outMin, outMax;
    
    // Get the extent of the forth dimension to be eliminated.
    axis = this->ExecutionAxes[dim - 1];
    inRegion1->GetAxisExtent(axis, inMin1, inMax1);
    outRegion->GetAxisExtent(axis, outMin, outMax);
    
    if (inRegion2)
      {
      inRegion2->GetAxisExtent(axis, inMin2, inMax2);
      if (inMin2 != inMin1 || inMax2 != inMax1)
	{
	vtkErrorMacro(<< "Execute: Extra axis can not be eliminated.(inputs)");
	return;
	}
      }
    
    // Extra axis of in and out must have the same extent
    if (inMin1 != outMin || inMax1 != outMax)
      {
      vtkErrorMacro(<< "Execute: Extra axis can not be eliminated. (output)");
      return;
      }
    
    // loop over the samples along the extra axis.
    for (coordinate = inMin1; coordinate <= inMax1; ++coordinate)
      {
      // set up the lower dimensional regions.
      inRegion1->SetAxisExtent(axis, coordinate, coordinate);
      if (inRegion2)
	{
	inRegion2->SetAxisExtent(axis, coordinate, coordinate);
	}
      outRegion->SetAxisExtent(axis, coordinate, coordinate);
      this->RecursiveLoopExecute(dim - 1, inRegion1, inRegion2, outRegion);
      }
    // restore the original extent
    inRegion1->SetAxisExtent(axis, inMin1, inMax1);
    if (inRegion2)
      {
      inRegion2->SetAxisExtent(axis, inMin2, inMax2);
      }
    outRegion->SetAxisExtent(axis, outMin, outMax);
    }
}

  
  
//----------------------------------------------------------------------------
// Description:
// The execute method created by the subclass.
void vtkImageTwoInputFilter::Execute(vtkImageRegion *inRegion1, 
				     vtkImageRegion *inRegion2, 
				     vtkImageRegion *outRegion)
{
  inRegion1 = inRegion2 = outRegion;
  vtkErrorMacro(<< "Subclass needs to supply an execute function.");
}

  







