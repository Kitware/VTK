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
  this->FilteredAxes[0] = VTK_IMAGE_X_AXIS;
  this->FilteredAxes[1] = VTK_IMAGE_Y_AXIS;
  this->FilteredAxes[2] = VTK_IMAGE_Z_AXIS;
  this->FilteredAxes[3] = VTK_IMAGE_TIME_AXIS;
  this->NumberOfFilteredAxes = 2;
  this->Input = NULL;
  this->SplitOrder[0] = VTK_IMAGE_COMPONENT_AXIS;
  this->SplitOrder[1] = VTK_IMAGE_TIME_AXIS;
  this->SplitOrder[2] = VTK_IMAGE_Z_AXIS;
  this->SplitOrder[3] = VTK_IMAGE_Y_AXIS;
  this->SplitOrder[4] = VTK_IMAGE_X_AXIS;
  this->NumberOfSplitAxes = 5;
  this->InputMemoryLimit = 5000000;   // 5 GB
  this->Bypass = 0;
  this->Updating = 0;
  // invalid settings
  this->NumberOfExecutionAxes = -1;
}

//----------------------------------------------------------------------------
void vtkImageFilter::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "Input: (" << this->Input << ").\n";
  os << indent << "SplitOrder: ("<< vtkImageAxisNameMacro(this->SplitOrder[0]);

  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << vtkImageAxisNameMacro(this->SplitOrder[idx]);
    }
  os << ")\n";

  vtkImageSource::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkImageFilter::SetFilteredAxes(int num, int *axes)
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
// source.  
unsigned long int vtkImageFilter::GetPipelineMTime()
{
  unsigned long int time1, time2;

  // This objects MTime
  // (Super class considers cache in case cache did not originate message)
  time1 = this->vtkImageSource::GetPipelineMTime();
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
// Set the Input of a filter. 
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
}


//----------------------------------------------------------------------------
// Description:
// This method is called by the cache.  It eventually calls the
// Execute(vtkImageRegion *, vtkImageRegion *) method.
// ImageInformation has already been updated by this point, 
// and outRegion is in local coordinates.
// This method will stream to get the input, and loops over extra axes.
// Only the UpdateExtent from output will get updated.
void vtkImageFilter::Update()
{
  vtkImageRegion *outRegion;

  // Make sure the Input has been set.
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Input is not set.");
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
    this->Input->SetUpdateExtent(this->Output->GetUpdateExtent());
    this->Input->Update();
    this->Output->SetScalarData(this->Input->GetScalarData());
    this->Output->SetNumberOfScalarComponents(
		      this->Input->GetNumberOfScalarComponents());
    // release input data
    if (this->Input->ShouldIReleaseData())
      {
      this->Input->ReleaseData();
      }
    this->Updating = 0;
    return;
    }
  
  // Make sure the subclss has defined the execute dimensionality
  // It is needed to terminate recursion.
  if (this->NumberOfExecutionAxes < 0)
    {
    vtkErrorMacro(<< "Subclass has not set NumberOfExecutionAxes");
    return;
    }

  // Get the output region.
  // Note: outRegion does not allocate until first "GetScalarPointer" call
  outRegion = this->Output->GetScalarRegion();
  outRegion->SetAxes(5, this->ExecutionAxes);

  // If outBBox is empty return immediately.
  if (outRegion->IsEmpty())
    {
    outRegion->Delete();
    this->Updating = 0;
    return;
    }

  // Fill in image information (ComputeRequiredInputExtent may need it)
  this->Input->UpdateImageInformation();
  
  // Pass on to a method that can be called recursively (for streaming).
  this->RecursiveStreamUpdate(outRegion);

  // free the output region (cache has a reference to the data.)
  outRegion->Delete();
  
  this->Updating = 0;
}



  
//----------------------------------------------------------------------------
// Description:
// This method can be called recursively for streaming.
// The extent of the outRegion changes, dim remains the same.
void vtkImageFilter::RecursiveStreamUpdate(vtkImageRegion *outRegion)
{
  int memory;
  vtkImageRegion *inRegion;

  //vtkTimerLogMacro("Entering Update");

  // Compute the required input region extent.
  // Copy to fill in extent of extra dimensions.
  this->Input->SetUpdateExtent(this->Output->GetUpdateExtent());
  this->ComputeRequiredInputUpdateExtent(this->Output, this->Input);
    
  // determine the amount of memory that will be used by the input region.
  memory = this->Input->GetUpdateExtentMemorySize();
  
  // Split the outRegion if we are streaming.
  if ((memory > this->InputMemoryLimit))
    {
    int splitAxisIdx, splitAxis;
    int min, max, mid;
    // We need to split the output into pieces.
    // It is convenient the outRegion has its own copy of output UpdateExtent.
    // Pick an axis to split.
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
      this->RecursiveStreamUpdate(outRegion);
      // Set the second half to update
      outRegion->SetAxisExtent(splitAxis, mid+1, max);
      this->RecursiveStreamUpdate(outRegion);
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
  // Get the input region (Update extent was set at start of this method).
  this->Input->Update();
  inRegion = this->Input->GetScalarRegion();
  inRegion->SetAxes(5, this->ExecutionAxes);
  // Make sure we got the input.
  if ( ! inRegion->AreScalarsAllocated())
    {
    vtkErrorMacro("RecursiveStreamUpdate: Could not get input");
    inRegion->Delete();
    return;
    }  
  
  // The StartMethod call is placed here to be after updating the input.
  if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
  // fill the output region 
  this->RecursiveLoopExecute(VTK_IMAGE_DIMENSIONS, inRegion, outRegion);
  if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
  
  // inRegion is just a handle to the data.
  inRegion->Delete();
  
  // Like the graphics pipeline this source releases inputs data.
  if (this->Input->ShouldIReleaseData())
    {
    this->Input->ReleaseData();
    }
}


//----------------------------------------------------------------------------
// Description:
// This method sets the WholeExtent, Spacing and Origin of the output.
void vtkImageFilter::UpdateImageInformation()
{
  // Make sure the Input has been set.
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "UpdateImageInformation: Input is not set.");
    return;
    }
  // make sure we have an output
  this->CheckCache();
  
  this->Input->UpdateImageInformation();
  // Set up the defaults
  this->Output->SetWholeExtent(this->Input->GetWholeExtent());
  this->Output->SetSpacing(this->Input->GetSpacing());
  this->Output->SetOrigin(this->Input->GetOrigin());
  this->Output->SetNumberOfScalarComponents(
			    this->Input->GetNumberOfScalarComponents());
  if ( ! this->Bypass)
    {
    // Let the subclass modify the default.
    this->ExecuteImageInformation(this->Input, this->Output);
    }
  
  // If the ScalarType of the output has not been set yet,
  // set it to be the same as input.
  if (this->Output->GetScalarType() == VTK_VOID)
    {
    this->Output->SetScalarType(this->Input->GetScalarType());
    }
}



//----------------------------------------------------------------------------
// Description:
// This method can be overriden in a subclass to compute the output
// ImageInformation: WholeExtent, Spacing and Origin.
void vtkImageFilter::ExecuteImageInformation(vtkImageCache *in,
					     vtkImageCache *out)
{
  // Default: Image information does not change (do nothing).
  // Avoid warnings
  in = in;
  out = out;
}



//----------------------------------------------------------------------------
// Description:
// This method can be overriden in a subclass to compute the input
// UpdateExtent needed to generate the output UpdateExtent.
// By default the input is set to the same as the output before this
// method is called.
void vtkImageFilter::ComputeRequiredInputUpdateExtent(vtkImageCache *out,
						      vtkImageCache *in)
{
  // avoid warnings
  in = in;
  out = out;
}



//----------------------------------------------------------------------------
// Description:
// This execute method recursively loops over extra dimensions and
// calls the subclasses Execute method with lower dimensional regions.
// NumberOfExecutionAxes is used to terminate the recursion.
void vtkImageFilter::RecursiveLoopExecute(int dim, vtkImageRegion *inRegion, 
					  vtkImageRegion *outRegion)
{
  // Terminate recursion?
  if (dim <= this->NumberOfExecutionAxes)
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
    axis = this->ExecutionAxes[dim - 1];
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

  
    











