/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePadFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImagePadFilter.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkImagePadFilter, "1.28.10.1");
vtkStandardNewMacro(vtkImagePadFilter);

//----------------------------------------------------------------------------
// Constructor sets default values
vtkImagePadFilter::vtkImagePadFilter()
{
  int idx;

  // Initialize output image extent to INVALID
  for (idx = 0; idx < 3; ++idx)
    {
    this->OutputWholeExtent[idx * 2] = 0;
    this->OutputWholeExtent[idx * 2 + 1] = -1;
    }
  // Set Output numberOfScalarComponents to INVALID
  this->OutputNumberOfScalarComponents = -1;
}

//----------------------------------------------------------------------------
void vtkImagePadFilter::SetOutputWholeExtent(int extent[6])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->OutputWholeExtent[idx] != extent[idx])
      {
      this->OutputWholeExtent[idx] = extent[idx];
      modified = 1;
      }
    }

  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImagePadFilter::SetOutputWholeExtent(int minX, int maxX, 
                                             int minY, int maxY,
                                             int minZ, int maxZ)
{
  int extent[6];
  
  extent[0] = minX;  extent[1] = maxX;
  extent[2] = minY;  extent[3] = maxY;
  extent[4] = minZ;  extent[5] = maxZ;
  this->SetOutputWholeExtent(extent);
}


//----------------------------------------------------------------------------
void vtkImagePadFilter::GetOutputWholeExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->OutputWholeExtent[idx];
    }
}


//----------------------------------------------------------------------------
// Just change the Image extent.
void vtkImagePadFilter::ExecuteInformation(
  vtkInformation       * vtkNotUsed( request ),
  vtkInformationVector * inputVector, 
  vtkInformationVector * outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = 
    this->GetInputConnectionInformation(inputVector,0,0);

  // we must set the extent on the input
  if (this->OutputWholeExtent[0] > this->OutputWholeExtent[1])
    {
    // invalid setting, it has not been set, so default to whole Extent
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 
                this->OutputWholeExtent);
    }
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 
               this->OutputWholeExtent, 6);

  if (this->OutputNumberOfScalarComponents < 0)
    {
    // invalid setting, it has not been set, so default to input.
    this->OutputNumberOfScalarComponents 
      = inInfo->Get(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS());
    }
  outInfo->Set(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS(),
               this->OutputNumberOfScalarComponents);
}

//----------------------------------------------------------------------------
// Just clip the request.  The subclass may need to overwrite this method.
void vtkImagePadFilter::ComputeInputUpdateExtent( 
  vtkInformation       * vtkNotUsed( request ),
  vtkInformationVector * inputVector, 
  vtkInformationVector * outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = 
    this->GetInputConnectionInformation(inputVector,0,0);

  // handle XYZ
  int uExt[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt);
  
  int inExt[6];
  memcpy(inExt,uExt,sizeof(int)*6);
  
  int wholeExtent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 
              wholeExtent);

  // Clip
  int idx;
  for (idx = 0; idx < 3; ++idx)
    {
    if (inExt[idx*2] < wholeExtent[idx*2])
      {
      inExt[idx*2] = wholeExtent[idx*2];
      }
    if (inExt[idx*2] > wholeExtent[idx*2 + 1])
      {
      inExt[idx*2] = wholeExtent[idx*2 + 1];
      }
    if (inExt[idx*2+1] < wholeExtent[idx*2])
      {
      inExt[idx*2+1] = wholeExtent[idx*2];
      }
    if (inExt[idx*2 + 1] > wholeExtent[idx*2 + 1])
      {
      inExt[idx*2 + 1] = wholeExtent[idx*2 + 1];
      }
    }
  
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), 
              inExt, 6);
}

void vtkImagePadFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OutputNumberOfScalarComponents: " 
     << this->OutputNumberOfScalarComponents << "\n";
}


