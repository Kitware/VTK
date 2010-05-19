/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencilSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageStencilSource.h"

#include "vtkImageStencilData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageStencilSource);

//----------------------------------------------------------------------------
vtkImageStencilSource::vtkImageStencilSource()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  vtkImageStencilData *output = vtkImageStencilData::New();
  this->GetExecutive()->SetOutputData(0, output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  output->ReleaseData();
  output->Delete();
}

//----------------------------------------------------------------------------
vtkImageStencilSource::~vtkImageStencilSource()
{
}

//----------------------------------------------------------------------------
void vtkImageStencilSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkImageStencilSource::SetOutput(vtkImageStencilData *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
vtkImageStencilData *vtkImageStencilSource::GetOutput()
{
  if (this->GetNumberOfOutputPorts() < 1)
    {
    return NULL;
    }
  
  return vtkImageStencilData::SafeDownCast(
    this->GetExecutive()->GetOutputData(0));
}

//----------------------------------------------------------------------------
vtkImageStencilData * 
vtkImageStencilSource::AllocateOutputData(vtkDataObject *out, int* uExt)
{
  vtkImageStencilData *res = vtkImageStencilData::SafeDownCast(out);
  if (!res)
    {
    vtkWarningMacro("Call to AllocateOutputData with non vtkImageStencilData"
                    " output");
    return NULL;
    }
  res->SetExtent(uExt);
  res->AllocateExtents();

  return res;
}  

//----------------------------------------------------------------------------
int vtkImageStencilSource::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject *out = outInfo->Get(vtkDataObject::DATA_OBJECT());
  this->AllocateOutputData(
    out,
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageStencilSource::FillOutputPortInformation(int,
                                                     vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageStencilData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageStencilSource::ProcessRequest(vtkInformation* request,
                                          vtkInformationVector** inputVector,
                                          vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    this->RequestData(request, inputVector, outputVector);
    return 1;
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    this->RequestInformation(request, inputVector, outputVector);
    return 1;
    }

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    this->RequestUpdateExtent(request, inputVector, outputVector);
    return 1;
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}
