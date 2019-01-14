/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredGridClip.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkStructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkStructuredGridClip);

//----------------------------------------------------------------------------
vtkStructuredGridClip::vtkStructuredGridClip()
{
  this->ClipData = 0;
  this->Initialized = 0;

  this->OutputWholeExtent[0] =
  this->OutputWholeExtent[2] =
  this->OutputWholeExtent[4] = -VTK_INT_MAX;

  this->OutputWholeExtent[1] =
  this->OutputWholeExtent[3] =
  this->OutputWholeExtent[5] = VTK_INT_MAX;
}

//----------------------------------------------------------------------------
void vtkStructuredGridClip::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  int idx;

  os << indent << "OutputWholeExtent: (" << this->OutputWholeExtent[0]
     << "," << this->OutputWholeExtent[1];
  for (idx = 1; idx < 3; ++idx)
  {
    os << indent << ", " << this->OutputWholeExtent[idx * 2]
       << "," << this->OutputWholeExtent[idx*2 + 1];
  }
  os << ")\n";
  if (this->ClipData)
  {
    os << indent << "ClipDataOn\n";
  }
  else
  {
    os << indent << "ClipDataOff\n";
  }
}

//----------------------------------------------------------------------------
void vtkStructuredGridClip::SetOutputWholeExtent(int extent[6], vtkInformation *outInfo)
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
    outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);
  }
}

//----------------------------------------------------------------------------
void vtkStructuredGridClip::SetOutputWholeExtent(int minX, int maxX,
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
void vtkStructuredGridClip::GetOutputWholeExtent(int extent[6])
{
  int idx;

  for (idx = 0; idx < 6; ++idx)
  {
    extent[idx] = this->OutputWholeExtent[idx];
  }
}

//----------------------------------------------------------------------------
// Change the WholeExtent
int vtkStructuredGridClip::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  int idx, extent[6];

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent);
  if ( ! this->Initialized)
  {
    this->SetOutputWholeExtent(extent, outInfo);
  }

  // Clip the OutputWholeExtent with the input WholeExtent
  for (idx = 0; idx < 3; ++idx)
  {
    if (this->OutputWholeExtent[idx*2] >= extent[idx*2] &&
        this->OutputWholeExtent[idx*2] <= extent[idx*2+1])
    {
      extent[idx*2] = this->OutputWholeExtent[idx*2];
    }
    if (this->OutputWholeExtent[idx*2+1] >= extent[idx*2] &&
        this->OutputWholeExtent[idx*2+1] <= extent[idx*2+1])
    {
      extent[idx*2+1] = this->OutputWholeExtent[idx*2+1];
    }
    // make usre the order is correct
    if (extent[idx*2] > extent[idx*2+1])
    {
      extent[idx*2] = extent[idx*2+1];
    }
  }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent,6);

  return 1;
}

//----------------------------------------------------------------------------
// Sets the output whole extent to be the input whole extent.
void vtkStructuredGridClip::ResetOutputWholeExtent()
{
  if ( ! this->GetInput())
  {
    vtkWarningMacro("ResetOutputWholeExtent: No input");
    return;
  }

  this->GetInputConnection(0, 0)->GetProducer()->UpdateInformation();
  vtkInformation *inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  this->SetOutputWholeExtent(inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int vtkStructuredGridClip::RequestData(vtkInformation *vtkNotUsed(request),
                               vtkInformationVector **inputVector,
                               vtkInformationVector *outputVector)
{
  int *inExt;
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkStructuredGrid *outData = vtkStructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkStructuredGrid *inData = vtkStructuredGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  inExt  = inData->GetExtent();

  outData->SetExtent(inExt);
  outData->GetPointData()->PassData(inData->GetPointData());
  outData->GetCellData()->PassData(inData->GetCellData());
  outData->SetPoints(inData->GetPoints());

  if (this->ClipData)
  {
    outData->Crop(
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));
  }

  return 1;
}
