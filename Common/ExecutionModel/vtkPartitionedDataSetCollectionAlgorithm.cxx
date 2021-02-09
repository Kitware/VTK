/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPartitionedDataSetCollectionCollectionAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"

//----------------------------------------------------------------------------
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
