/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSpatialFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageSpatialFilter.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include <math.h>

vtkStandardNewMacro(vtkImageSpatialFilter);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageSpatialFilter fitler.
vtkImageSpatialFilter::vtkImageSpatialFilter()
{
  this->KernelSize[0] = this->KernelSize[1] = this->KernelSize[2] = 1;
  this->KernelMiddle[0] = this->KernelMiddle[1] = this->KernelMiddle[2] = 0;
  this->HandleBoundaries = 1;
}


//----------------------------------------------------------------------------
void vtkImageSpatialFilter::PrintSelf(ostream& os, vtkIndent indent)
{  
  this->Superclass::PrintSelf(os, indent);
  
  int idx;

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

  vtkDataArray *inArray;
  inArray = input->GetPointData()->GetScalars(this->InputScalarsSelection);
  if (inArray)
    {
    output->SetScalarType(inArray->GetDataType());
    output->SetNumberOfScalarComponents(inArray->GetNumberOfComponents());
    }
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


