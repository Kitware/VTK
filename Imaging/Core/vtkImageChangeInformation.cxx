/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageChangeInformation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageChangeInformation.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkImageChangeInformation);

//----------------------------------------------------------------------------
vtkImageChangeInformation::vtkImageChangeInformation()
{
  this->CenterImage = 0;

  for (int i = 0; i < 3; i++)
  {
    this->OutputExtentStart[i] = VTK_INT_MAX;
    this->ExtentTranslation[i] = 0;
    this->FinalExtentTranslation[i] = VTK_INT_MAX;

    this->OutputSpacing[i] = VTK_DOUBLE_MAX;
    this->SpacingScale[i] = 1.0;

    this->OutputOrigin[i] = VTK_DOUBLE_MAX;
    this->OriginScale[i] = 1.0;
    this->OriginTranslation[i] = 0.0;
  }

  // There is an optional second input.
  this->SetNumberOfInputPorts(2);
}

// Specify a source object at a specified table location.
void vtkImageChangeInformation::SetInformationInputData(vtkImageData *pd)
{
  this->SetInputData(1, pd);
}

// Get a pointer to a source object at a specified table location.
vtkImageData *vtkImageChangeInformation::GetInformationInput()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return 0;
  }
  return vtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
vtkImageChangeInformation::~vtkImageChangeInformation()
{
}

//----------------------------------------------------------------------------
void vtkImageChangeInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CenterImage : "
     << (this->CenterImage ? "On":"Off") << endl;

  os << indent << "OutputExtentStart: ("
     << this->OutputExtentStart[0] << ","
     << this->OutputExtentStart[1] << ","
     << this->OutputExtentStart[2] << ")" << endl;

  os << indent << "ExtentTranslation: ("
     << this->ExtentTranslation[0] << ","
     << this->ExtentTranslation[1] << ","
     << this->ExtentTranslation[2] << ")" << endl;

  os << indent << "OutputSpacing: ("
     << this->OutputSpacing[0] << ","
     << this->OutputSpacing[1] << ","
     << this->OutputSpacing[2] << ")" << endl;

  os << indent << "SpacingScale: ("
     << this->SpacingScale[0] << ","
     << this->SpacingScale[1] << ","
     << this->SpacingScale[2] << ")" << endl;

  os << indent << "OutputOrigin: ("
     << this->OutputOrigin[0] << ","
     << this->OutputOrigin[1] << ","
     << this->OutputOrigin[2] << ")" << endl;

  os << indent << "OriginScale: ("
     << this->OriginScale[0] << ","
     << this->OriginScale[1] << ","
     << this->OriginScale[2] << ")" << endl;

  os << indent << "OriginTranslation: ("
     << this->OriginTranslation[0] << ","
     << this->OriginTranslation[1] << ","
     << this->OriginTranslation[2] << ")" << endl;
}

//----------------------------------------------------------------------------
// Change the information
int vtkImageChangeInformation::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  int i;
  int extent[6], inExtent[6];
  double spacing[3], origin[3];

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),inExtent);

  vtkImageData *infoInput = this->GetInformationInput();
  if (infoInput)
  {
    // If there is an InformationInput, it is set as a second input
    vtkInformation *in2Info = inputVector[1]->GetInformationObject(0);
    infoInput->GetOrigin(origin);
    infoInput->GetSpacing(spacing);
    in2Info->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent);
    for (i = 0; i < 3; i++)
    {
      extent[2*i+1] = extent[2*i] - inExtent[2*i] + inExtent[2*i+1];
    }
  }
  else
  {
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent);
    inInfo->Get(vtkDataObject::ORIGIN(), origin);
    inInfo->Get(vtkDataObject::SPACING(), spacing);
  }

  for (i = 0; i < 3; i++)
  {
    if (this->OutputSpacing[i] != VTK_DOUBLE_MAX)
    {
      spacing[i] = this->OutputSpacing[i];
    }

    if (this->OutputOrigin[i] != VTK_DOUBLE_MAX)
    {
      origin[i] = this->OutputOrigin[i];
    }

    if (this->OutputExtentStart[i] != VTK_INT_MAX)
    {
      extent[2*i+1] += this->OutputExtentStart[i] - extent[2*i];
      extent[2*i] = this->OutputExtentStart[i];
    }
  }

  if (this->CenterImage)
  {
    for (i = 0; i < 3; i++)
    {
      origin[i] = -(extent[2*i] + extent[2*i+1])*spacing[i]/2;
    }
  }

  for (i = 0; i < 3; i++)
  {
    spacing[i] = spacing[i]*this->SpacingScale[i];
    origin[i] = origin[i]*this->OriginScale[i] + this->OriginTranslation[i];
    extent[2*i] = extent[2*i] + this->ExtentTranslation[i];
    extent[2*i+1] = extent[2*i+1] + this->ExtentTranslation[i];
    this->FinalExtentTranslation[i] = extent[2*i] - inExtent[2*i];
  }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent,6);
  outInfo->Set(vtkDataObject::SPACING(),spacing,3);
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);

  return 1;
}


//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int vtkImageChangeInformation::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (this->FinalExtentTranslation[0] == VTK_INT_MAX)
  {
    vtkErrorMacro("Bug in code, RequestInformation was not called");
    return 0;
  }

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int extent[6];

  vtkImageData *inData = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData *outData = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // since inData can be larger than update extent.
  inData->GetExtent(extent);
  for (int i = 0; i < 3; ++i)
  {
    extent[i*2] += this->FinalExtentTranslation[i];
    extent[i*2+1] += this->FinalExtentTranslation[i];
  }
  outData->SetExtent(extent);
  outData->GetPointData()->PassData(inData->GetPointData());

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageChangeInformation::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  if (this->FinalExtentTranslation[0] == VTK_INT_MAX)
  {
    vtkErrorMacro("Bug in code.");
    return 0;
  }

  int inExt[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);

  inExt[0] -= this->FinalExtentTranslation[0];
  inExt[1] -= this->FinalExtentTranslation[0];
  inExt[2] -= this->FinalExtentTranslation[1];
  inExt[3] -= this->FinalExtentTranslation[1];
  inExt[4] -= this->FinalExtentTranslation[2];
  inExt[5] -= this->FinalExtentTranslation[2];

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  return 1;
}

int vtkImageChangeInformation::FillInputPortInformation(
  int port, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}
