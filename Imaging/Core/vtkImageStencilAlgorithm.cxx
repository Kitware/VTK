// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageStencilAlgorithm.h"

#include "vtkImageStencilData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageStencilAlgorithm);

//------------------------------------------------------------------------------
vtkImageStencilAlgorithm::vtkImageStencilAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  vtkImageStencilData* output = vtkImageStencilData::New();
  this->GetExecutive()->SetOutputData(0, output);

  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();
}

//------------------------------------------------------------------------------
vtkImageStencilAlgorithm::~vtkImageStencilAlgorithm() = default;

//------------------------------------------------------------------------------
void vtkImageStencilAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkImageStencilAlgorithm::SetOutput(vtkImageStencilData* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//------------------------------------------------------------------------------
vtkImageStencilData* vtkImageStencilAlgorithm::GetOutput()
{
  if (this->GetNumberOfOutputPorts() < 1)
  {
    return nullptr;
  }

  return vtkImageStencilData::SafeDownCast(this->GetExecutive()->GetOutputData(0));
}

//------------------------------------------------------------------------------
vtkImageStencilData* vtkImageStencilAlgorithm::AllocateOutputData(vtkDataObject* out, int* uExt)
{
  vtkImageStencilData* res = vtkImageStencilData::SafeDownCast(out);
  if (!res)
  {
    vtkWarningMacro("Call to AllocateOutputData with non vtkImageStencilData"
                    " output");
    return nullptr;
  }
  res->SetExtent(uExt);
  res->AllocateExtents();

  return res;
}

//------------------------------------------------------------------------------
int vtkImageStencilAlgorithm::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* out = outInfo->Get(vtkDataObject::DATA_OBJECT());
  this->AllocateOutputData(out, outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));

  return 1;
}

//------------------------------------------------------------------------------
int vtkImageStencilAlgorithm::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  return 1;
}

//------------------------------------------------------------------------------
int vtkImageStencilAlgorithm::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  return 1;
}

//------------------------------------------------------------------------------
int vtkImageStencilAlgorithm::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageStencilData");
  return 1;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkImageStencilAlgorithm::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->RequestData(request, inputVector, outputVector);
    return 1;
  }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
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
VTK_ABI_NAMESPACE_END
