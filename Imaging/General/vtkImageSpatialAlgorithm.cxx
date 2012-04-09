/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSpatialAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageSpatialAlgorithm.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageSpatialAlgorithm);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageSpatialAlgorithm fitler.
vtkImageSpatialAlgorithm::vtkImageSpatialAlgorithm()
{
  this->KernelSize[0] = this->KernelSize[1] = this->KernelSize[2] = 1;
  this->KernelMiddle[0] = this->KernelMiddle[1] = this->KernelMiddle[2] = 0;
  this->HandleBoundaries = 1;
}


//----------------------------------------------------------------------------
void vtkImageSpatialAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
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
int vtkImageSpatialAlgorithm::RequestInformation (
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  // Take this opportunity to override the defaults. 
  int extent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  this->ComputeOutputWholeExtent(extent, this->HandleBoundaries);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  return 1;
}

//----------------------------------------------------------------------------
// A helper method to compute output image extent
void vtkImageSpatialAlgorithm::ComputeOutputWholeExtent(int extent[6], 
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
int vtkImageSpatialAlgorithm::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int wholeExtent[6], extent[6], inExtent[6];
  
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExtent);

  this->InternalRequestUpdateExtent(extent, inExtent, wholeExtent);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);

  return 1;
}

//----------------------------------------------------------------------------
void vtkImageSpatialAlgorithm::InternalRequestUpdateExtent(int *extent,
                                                           int *inExtent,
                                                           int *wholeExtent)
{
  int idx;
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
