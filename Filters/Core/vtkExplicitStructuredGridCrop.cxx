/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExplicitStructuredGridCrop.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExplicitStructuredGridCrop.h"

#include "vtkAlgorithmOutput.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkExplicitStructuredGridCrop);

//-----------------------------------------------------------------------------
vtkExplicitStructuredGridCrop::vtkExplicitStructuredGridCrop()
{
  this->Initialized = 0;
  this->OutputWholeExtent[0] = this->OutputWholeExtent[2] = this->OutputWholeExtent[4] =
    VTK_INT_MIN;

  this->OutputWholeExtent[1] = this->OutputWholeExtent[3] = this->OutputWholeExtent[5] =
    VTK_INT_MAX;
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGridCrop::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutputWholeExtent: (" << this->OutputWholeExtent[0] << ","
     << this->OutputWholeExtent[1];
  os << indent << ", " << this->OutputWholeExtent[2] << "," << this->OutputWholeExtent[3];
  os << indent << ", " << this->OutputWholeExtent[4] << "," << this->OutputWholeExtent[5];
  os << ")\n";
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGridCrop::SetOutputWholeExtent(int extent[6], vtkInformation* outInfo)
{
  int idx;
  int modified = 0;

  for (idx = 0; idx < 6; ++idx)
  {
    if (this->OutputWholeExtent[idx] != extent[idx])
    {
      this->OutputWholeExtent[idx] = extent[idx];
      modified = 1;
    }
  }
  this->Initialized = 1;
  if (modified)
  {
    this->Modified();
    if (!outInfo)
    {
      outInfo = this->GetExecutive()->GetOutputInformation(0);
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
  }
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGridCrop::SetOutputWholeExtent(
  int minX, int maxX, int minY, int maxY, int minZ, int maxZ)
{
  int extent[6];
  extent[0] = minX;
  extent[1] = maxX;
  extent[2] = minY;
  extent[3] = maxY;
  extent[4] = minZ;
  extent[5] = maxZ;
  this->SetOutputWholeExtent(extent);
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGridCrop::GetOutputWholeExtent(int extent[6])
{
  for (int idx = 0; idx < 6; ++idx)
  {
    extent[idx] = this->OutputWholeExtent[idx];
  }
}

//----------------------------------------------------------------------------
// Sets the output whole extent to be the input whole extent.
void vtkExplicitStructuredGridCrop::ResetOutputWholeExtent()
{
  if (!this->GetInput())
  {
    vtkWarningMacro("ResetOutputWholeExtent: No input");
    return;
  }

  this->GetInputConnection(0, 0)->GetProducer()->UpdateInformation();
  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  this->SetOutputWholeExtent(inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
}

//----------------------------------------------------------------------------
// Change the WholeExtent
int vtkExplicitStructuredGridCrop::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int extent[6];

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

  if (!this->Initialized)
  {
    this->SetOutputWholeExtent(extent, outInfo);
  }

  // Clip the OutputWholeExtent with the input WholeExtent
  for (int idx = 0; idx < 3; ++idx)
  {
    if (this->OutputWholeExtent[idx * 2] >= extent[idx * 2] &&
      this->OutputWholeExtent[idx * 2] <= extent[idx * 2 + 1])
    {
      extent[idx * 2] = this->OutputWholeExtent[idx * 2];
    }
    if (this->OutputWholeExtent[idx * 2 + 1] >= extent[idx * 2] &&
      this->OutputWholeExtent[idx * 2 + 1] <= extent[idx * 2 + 1])
    {
      extent[idx * 2 + 1] = this->OutputWholeExtent[idx * 2 + 1];
    }
  }
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
  return 1;
}

//----------------------------------------------------------------------------
int vtkExplicitStructuredGridCrop::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  // We can handle anything.
  vtkInformation* info = inputVector[0]->GetInformationObject(0);
  info->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 0);
  return 1;
}

//----------------------------------------------------------------------------
int vtkExplicitStructuredGridCrop::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Retrieve input and output
  vtkExplicitStructuredGrid* input = vtkExplicitStructuredGrid::GetData(inputVector[0], 0);
  vtkExplicitStructuredGrid* output = vtkExplicitStructuredGrid::GetData(outputVector, 0);

  output->Crop(input, this->OutputWholeExtent, true);

  this->UpdateProgress(1.);
  return 1;
}
