// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"

//----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkPartitionedDataSetCollectionAlgorithm::vtkPartitionedDataSetCollectionAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPartitionedDataSetCollectionAlgorithm::~vtkPartitionedDataSetCollectionAlgorithm() = default;

//----------------------------------------------------------------------------
vtkPartitionedDataSetCollection* vtkPartitionedDataSetCollectionAlgorithm::GetOutput()
{
  return vtkPartitionedDataSetCollection::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
vtkPartitionedDataSetCollection* vtkPartitionedDataSetCollectionAlgorithm::GetOutput(int port)
{
  return vtkPartitionedDataSetCollection::SafeDownCast(this->GetOutputDataObject(port));
}

//------------------------------------------------------------------------------
vtkTypeBool vtkPartitionedDataSetCollectionAlgorithm::ProcessRequest(
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

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkPartitionedDataSetCollectionAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection");
  return 1;
}

//------------------------------------------------------------------------------
int vtkPartitionedDataSetCollectionAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetCollectionAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
