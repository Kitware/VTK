/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSpatialFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>

#include "vtkImageSpatialFilter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageSpatialFilter* vtkImageSpatialFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageSpatialFilter");
  if(ret)
    {
    return (vtkImageSpatialFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageSpatialFilter;
}





//----------------------------------------------------------------------------
// Construct an instance of vtkImageSpatialFilter fitler.
vtkImageSpatialFilter::vtkImageSpatialFilter()
{
  int idx;
  
  for (idx = 0; idx < 3; ++idx)
    {
    this->KernelSize[idx] = 1;
    this->KernelMiddle[idx] = 0;
    }
  
  this->HandleBoundaries = 1;
}


//----------------------------------------------------------------------------
void vtkImageSpatialFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkImageToImageFilter::PrintSelf(os, indent);

  os << indent << "KernelSize: (" << this->KernelSize[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->KernelSize[idx];
    }
  os << ").\n";

  os << indent << "KernelMiddle: (" << this->KernelMiddle[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->KernelMiddle[idx];
    }
  os << ").\n";

}



//----------------------------------------------------------------------------
// This method is passed a region that holds the image extent of this filters
// input, and changes the region to hold the image extent of this filters
// output.
void vtkImageSpatialFilter::ExecuteInformation()
{
  vtkImageData *input = this->GetInput();
  vtkImageData *output = this->GetOutput();
  
  if (!input)
    {
    vtkErrorMacro(<< "Input not set.");
    return;
    }
  // Copy the defaults
  output->CopyTypeSpecificInformation( input );

  // Take this opportunity to override the defaults. 
  int extent[6];
  input->GetWholeExtent(extent);
  this->ComputeOutputWholeExtent(extent, this->HandleBoundaries);
  output->SetWholeExtent(extent);
  this->ExecuteInformation(input, output);
}
//----------------------------------------------------------------------------
void vtkImageSpatialFilter::ExecuteInformation(
           vtkImageData *vtkNotUsed(inData), vtkImageData *vtkNotUsed(outData))
{
}

//----------------------------------------------------------------------------
// A helper method to compute output image extent
void vtkImageSpatialFilter::ComputeOutputWholeExtent(int extent[6], 
						     int handleBoundaries)
{
  int idx;

  if ( ! handleBoundaries)
    {
    // Make extent a little smaller because of the kernel size.
    for (idx = 0; idx < 3; ++idx)
      {
      extent[idx*2] += this->KernelMiddle[idx];
      extent[idx*2+1] -= (this->KernelSize[idx]-1) - this->KernelMiddle[idx];
      }
    }
  
}



//----------------------------------------------------------------------------
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// extent of the output region.  After this method finishes, "region" should 
// have the extent of the required input region.
void vtkImageSpatialFilter::ComputeInputUpdateExtent(int extent[6], 
						     int inExtent[6])
{
  int idx;
  int *wholeExtent;
  
  if (!this->GetInput())
    {
    return;
    }

  wholeExtent = this->GetInput()->GetWholeExtent();
  for (idx = 0; idx < 3; ++idx)
    {
    // Magnify by strides
    extent[idx*2] = inExtent[idx*2];
    extent[idx*2+1] = inExtent[idx*2+1];

    // Expand to get inRegion Extent
    extent[idx*2] -= this->KernelMiddle[idx];
    extent[idx*2+1] += (this->KernelSize[idx]-1) - this->KernelMiddle[idx];
    
    // If the expanded region is out of the IMAGE Extent (grow min)
    if (extent[idx*2] < wholeExtent[idx*2])
      {
      if (this->HandleBoundaries)
	{
	// shrink the required region extent
	extent[idx*2] = wholeExtent[idx*2];
	}
      else
	{
	vtkWarningMacro(<< "Required region is out of the image extent.");
	}
      }
    // If the expanded region is out of the IMAGE Extent (shrink max)      
    if (extent[idx*2+1] > wholeExtent[idx*2+1])
      {
      if (this->HandleBoundaries)
	{
	// shrink the required region extent
	extent[idx*2+1] = wholeExtent[idx*2+1];
	}
      else
	{
	vtkWarningMacro(<< "Required region is out of the image extent.");
	}
      }
    }
}


