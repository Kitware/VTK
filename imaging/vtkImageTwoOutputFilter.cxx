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
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageTwoOutputFilter.h"

//----------------------------------------------------------------------------
vtkImageTwoOutputFilter::vtkImageTwoOutputFilter()
{
  this->FilteredAxes[0] = VTK_IMAGE_X_AXIS;
  this->FilteredAxes[1] = VTK_IMAGE_Y_AXIS;
  this->FilteredAxes[2] = VTK_IMAGE_Z_AXIS;
  this->FilteredAxes[3] = VTK_IMAGE_TIME_AXIS;
  this->NumberOfFilteredAxes = 2;
  this->Input = NULL;
  this->Output2 = NULL;
  this->Bypass = 0;
  this->Updating = 0;
  
  // invalid settings
  this->NumberOfExecutionAxes = -1;
}

//----------------------------------------------------------------------------
void vtkImageTwoOutputFilter::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "Output2: (" << this->Output2 << ").\n";
  vtkImageSource::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkImageTwoOutputFilter::SetFilteredAxes(int num, int *axes)
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
}


//----------------------------------------------------------------------------
// Description:
// This method is called by the cache.  
void vtkImageTwoOutputFilter::Update()
{
  vtkImageRegion *inRegion;
  vtkImageRegion *outRegion1;
  vtkImageRegion *outRegion2;

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
  this->CheckCache2();

  // In case this update is called directly.
  this->UpdateImageInformation();
  this->Output->ClipUpdateExtentWithWholeExtent();
  this->Output2->ClipUpdateExtentWithWholeExtent();

  // Handle bypass condition.
  if (this->Bypass)
    {
    this->Input->SetUpdateExtent(this->Output->GetUpdateExtent());
    this->Input->Update();
    this->Output->SetScalarData(this->Input->GetScalarData());
    this->Output->SetNumberOfScalarComponents(
		      this->Input->GetNumberOfScalarComponents());
    this->Output2->SetScalarData(this->Input->GetScalarData());
    this->Output2->SetNumberOfScalarComponents(
		       this->Input->GetNumberOfScalarComponents());
    // release input data
    if (this->Input->ShouldIReleaseData())
      {
      this->Input->ReleaseData();
      }
    this->Updating = 0;
    return;
    }
  
  // Make sure the subclass has defined the execute dimensionality.
  if (this->NumberOfExecutionAxes < 0)
    {
    vtkErrorMacro(<< "Subclass has not set NumberOfExecutionAxes");
    this->Updating = 0;
    return;
    }

  // Get the output region.
  // Note: outRegion does not allocate until first "GetScalarPointer" call
  outRegion1 = this->Output->GetScalarRegion();
  outRegion1->SetAxes(5, this->ExecutionAxes);
  outRegion2 = this->Output->GetScalarRegion();
  outRegion2->SetAxes(5, this->ExecutionAxes);
  
  // If outBBox is empty return imediately.
  if (outRegion1->IsEmpty() && outRegion2->IsEmpty())
    {
    outRegion1->Delete();
    outRegion2->Delete();
    this->Updating = 0;
    return;
    }
    
  // Fill in image information (ComputeRequiredInputExtent may need it)
  this->Input->UpdateImageInformation();
  this->Input->SetUpdateExtent(this->Output->GetUpdateExtent());
  this->ComputeRequiredInputUpdateExtent();
  
  // ... no streaming implemented yet ...

  // Get the input regions
  this->Input->Update();
  inRegion = this->Input->GetScalarRegion();
  inRegion->SetAxes(5, this->ExecutionAxes);
  // Make sure we got the input2.
  if ( ! inRegion->AreScalarsAllocated())
    {
    vtkErrorMacro("Update: Could not get input");
    inRegion->Delete();
    outRegion1->Delete();
    outRegion2->Delete();
    this->Updating = 0;
    return;
    }     
  
  // The StartMethod call is placed here to be after updating the input.
  if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
  // fill the output region 
  this->RecursiveLoopExecute(VTK_IMAGE_DIMENSIONS, inRegion, outRegion1, 
			     outRegion2);
  if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
  
  
  // inRegion is just a handle to the data.
  inRegion->Delete();
  
  // Like the graphics pipeline this source releases inputs data.
  if (this->Input->ShouldIReleaseData())
    {
    this->Input->ReleaseData();
    }
  
  // Delete the container for the data (not the data).
  outRegion1->Delete();
  outRegion2->Delete();
  
  this->Updating = 0;
}


  
//----------------------------------------------------------------------------
// Description:
// This execute method recursively loops over extra dimensions and
// calls the subclasses Execute method with lower dimensional regions.
void vtkImageTwoOutputFilter::RecursiveLoopExecute(int dim, 
						   vtkImageRegion *inRegion, 
						   vtkImageRegion *outRegion1,
						   vtkImageRegion *outRegion2)
{
  // Terminate recursion?
  if (dim <= this->NumberOfExecutionAxes)
    {
    this->Execute(inRegion, outRegion1, outRegion2);
    return;
    }
  else
    {
    int coordinate, axis;
    int inMin, inMax;
    int outMin1, outMax1;
    int outMin2, outMax2;
    
    // Get the extent of the dimension to be eliminated.
    axis = this->ExecutionAxes[dim - 1];
    inRegion->GetAxisExtent(axis, inMin, inMax);
    outRegion1->GetAxisExtent(axis, outMin1, outMax1);
    outRegion2->GetAxisExtent(axis, outMin2, outMax2);

    // The axis should have the same extent.
    if (inMin!=outMin1 || inMax!=outMax1 || inMin!=outMin2 || inMax!=outMax2) 
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
      outRegion1->SetAxisExtent(axis, coordinate, coordinate);
      outRegion2->SetAxisExtent(axis, coordinate, coordinate);
      this->RecursiveLoopExecute(dim - 1, inRegion, 
				 outRegion1, outRegion2);
      }
    // restore the original extent
    inRegion->SetAxisExtent(axis, inMin, inMax);
    outRegion1->SetAxisExtent(axis, outMin1, outMax1);
    outRegion2->SetAxisExtent(axis, outMin2, outMax2);
    }
}










//----------------------------------------------------------------------------
// Description:
// This method gets the boundary of the input then computes and returns 
// the boundary of the largest region that can be generated. 
void vtkImageTwoOutputFilter::UpdateImageInformation()
{
  // Make sure the Input has been set.
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "UpdateImageInformation: Input is not set.");
    return;
    }
  // make sure we have outputs
  this->CheckCache();
  this->CheckCache2();
    
  this->Input->UpdateImageInformation();
  // Set the defaults from input
  this->Output->SetWholeExtent(this->Input->GetWholeExtent());
  this->Output->SetSpacing(this->Input->GetSpacing());
  this->Output->SetOrigin(this->Input->GetOrigin());
  this->Output2->SetWholeExtent(this->Input->GetWholeExtent());
  this->Output2->SetSpacing(this->Input->GetSpacing());
  this->Output2->SetOrigin(this->Input->GetOrigin());
  if ( ! this->Bypass)
    {
    // Let the subclass modify the default.
    this->ExecuteImageInformation();
    }

  // If the ScalarType of the outputs have not been set yet,
  // set them to be the same as input.
  if (this->Output->GetScalarType() == VTK_VOID)
    {
    this->Output->SetScalarType(this->Input->GetScalarType());
    }
  if (this->Output2->GetScalarType() == VTK_VOID)
    {
    this->Output2->SetScalarType(this->Input->GetScalarType());
    }
}



//----------------------------------------------------------------------------
// Description:
// Given a region with input image info, compute output image info.
// Image info should be the same for both outputs!
void vtkImageTwoOutputFilter::ExecuteImageInformation()
{
}



//----------------------------------------------------------------------------
// Description:
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "input" should have the 
// UpdateExtent of output0.  After this method finishes, "input" should 
// have the UpdateExtent of the required input extent.
void vtkImageTwoOutputFilter::ComputeRequiredInputUpdateExtent()
{
}



//----------------------------------------------------------------------------
// Description:
// The execute method created by the subclass.
void vtkImageTwoOutputFilter::Execute(vtkImageRegion *inRegion, 
				      vtkImageRegion *outRegion1,
				      vtkImageRegion *outRegion2)
{
  inRegion = outRegion1 = outRegion2;
  vtkErrorMacro(<< "Subclass needs to suply an execute function.");
}



//----------------------------------------------------------------------------
// Description:
// Returns the cache object of the source.  If one does not exist, a default
// is created.
vtkImageCache *vtkImageTwoOutputFilter::GetOutput2()
{
  this->CheckCache2();
  
  return this->Output2;
}


//----------------------------------------------------------------------------
// Description:
// This private method creates a cache if one has not been set.
// ReleaseDataFlag is turned on.
void vtkImageTwoOutputFilter::CheckCache2()
{
  // create a default cache if one has not been set
  if ( ! this->Output2)
    {
    this->Output2 = vtkImageCache::New();
    this->Output2->ReleaseDataFlagOn();
    this->Output2->SetSource(this);
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
  this->CheckCache2();
  this->Output->SetReleaseDataFlag(value);
  this->Output2->SetReleaseDataFlag(value);
}







