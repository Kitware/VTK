// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPartitionedDataSetAlgorithm.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"

//----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkPartitionedDataSetAlgorithm::vtkPartitionedDataSetAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPartitionedDataSetAlgorithm::~vtkPartitionedDataSetAlgorithm() = default;

//----------------------------------------------------------------------------
vtkPartitionedDataSet* vtkPartitionedDataSetAlgorithm::GetOutput()
{
  return vtkPartitionedDataSet::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
vtkPartitionedDataSet* vtkPartitionedDataSetAlgorithm::GetOutput(int port)
{
  return vtkPartitionedDataSet::SafeDownCast(this->GetOutputDataObject(port));
}

//------------------------------------------------------------------------------
vtkTypeBool vtkPartitionedDataSetAlgorithm::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  // generate the data
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
  {
    int retVal = this->RequestData(request, inputVector, outputVector);
    return retVal;
  }

  // execute information
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  // set update extent
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_TIME()))
  {
    return this->RequestUpdateTime(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkPartitionedDataSetAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkPartitionedDataSetAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
