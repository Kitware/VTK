/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToImageStencil.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageToImageStencil.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkImageStencilData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageToImageStencil);

//----------------------------------------------------------------------------
vtkImageToImageStencil::vtkImageToImageStencil()
{
  this->UpperThreshold = VTK_LARGE_FLOAT;
  this->LowerThreshold = -VTK_LARGE_FLOAT;
}

//----------------------------------------------------------------------------
vtkImageToImageStencil::~vtkImageToImageStencil()
{
}

//----------------------------------------------------------------------------
void vtkImageToImageStencil::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Input: " << this->GetInput() << "\n";
  os << indent << "UpperThreshold: " << this->UpperThreshold << "\n";
  os << indent << "LowerThreshold: " << this->LowerThreshold << "\n";
}

//----------------------------------------------------------------------------
void vtkImageToImageStencil::SetInput(vtkImageData *input)
{
  if (input)
    {
    this->SetInputConnection(0, input->GetProducerPort());
    }
  else
    {
    this->SetInputConnection(0, 0);
    }
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageToImageStencil::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return NULL;
    }
  
  return vtkImageData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
// The values greater than or equal to the value match.
void vtkImageToImageStencil::ThresholdByUpper(double thresh)
{
  if (this->LowerThreshold != thresh || this->UpperThreshold < VTK_LARGE_FLOAT)
    {
    this->LowerThreshold = thresh;
    this->UpperThreshold = VTK_LARGE_FLOAT;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// The values less than or equal to the value match.
void vtkImageToImageStencil::ThresholdByLower(double thresh)
{
  if (this->UpperThreshold != thresh || this->LowerThreshold > -VTK_LARGE_FLOAT)
    {
    this->UpperThreshold = thresh;
    this->LowerThreshold = -VTK_LARGE_FLOAT;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// The values in a range (inclusive) match
void vtkImageToImageStencil::ThresholdBetween(double lower, double upper)
{
  if (this->LowerThreshold != lower || this->UpperThreshold != upper)
    {
    this->LowerThreshold = lower;
    this->UpperThreshold = upper;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkImageToImageStencil::RequestData(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *inData = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageStencilData *data = vtkImageStencilData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int extent[6];
  inData->GetExtent(extent);
  // output extent is always the input extent
  this->AllocateOutputData(data, extent);

  vtkDataArray *inScalars = inData->GetPointData()->GetScalars();
  double upperThreshold = this->UpperThreshold;
  double lowerThreshold = this->LowerThreshold;

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>(
    (extent[5] - extent[4] + 1)*(extent[3] - extent[2] + 1)/50.0);
  target++;

  for (int idZ = extent[4]; idZ <= extent[5]; idZ++)
    {
    for (int idY = extent[2]; idY <= extent[3]; idY++)
      {
      if (count%target == 0) 
        {
        this->UpdateProgress(count/(50.0*target));
        }
      count++;

      int state = 1; // inside or outside, start outside
      int r1 = extent[0];
      int r2 = extent[1];

      // index into scalar array
      int idS = ((extent[1] - extent[0] + 1)*
                 ((extent[3] - extent[2] + 1)*(idZ - extent[4]) +
                  (idY - extent[2])));

      for (int idX = extent[0]; idX <= extent[1]; idX++)
        {
        int newstate = 1;
        double value = inScalars->GetComponent(idS++,0);
        if (value >= lowerThreshold && value <= upperThreshold)
          {
          newstate = -1;
          if (newstate != state)
            { // sub extent starts
            r1 = idX;
            }
          }
        else if (newstate != state)
          { // sub extent ends
          r2 = idX - 1;
          data->InsertNextExtent(r1, r2, idY, idZ);
          }
        state = newstate;
        } // for idX
      if (state < 0)
        { // if inside at end, cap off the sub extent
        data->InsertNextExtent(r1, extent[1], idY, idZ);
        }
      } // for idY
    } // for idZ

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageToImageStencil::RequestInformation(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // whole extent is largest possible extent, because this filter
  // can accommodate any update extent
  static int wholeExtent[6] = { 0, VTK_LARGE_INTEGER >> 2,
                                0, VTK_LARGE_INTEGER >> 2,
                                0, VTK_LARGE_INTEGER >> 2 };
  double spacing[3];
  double origin[3];

  inInfo->Get(vtkDataObject::SPACING(), spacing);
  inInfo->Get(vtkDataObject::ORIGIN(), origin);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               wholeExtent, 6);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageToImageStencil::FillInputPortInformation(int,
                                                     vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageToImageStencil::RequestUpdateExtent(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
              inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
              6);
  return 1;
}
