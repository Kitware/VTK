/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataSetAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiBlockDataSetAlgorithm.h"

#include "vtkCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkMultiBlockDataSetAlgorithm, "1.2");
vtkStandardNewMacro(vtkMultiBlockDataSetAlgorithm);

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
vtkMultiBlockDataSetAlgorithm::vtkMultiBlockDataSetAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkMultiBlockDataSetAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkMultiBlockDataSetAlgorithm::GetOutput(int port)
{
  vtkDataObject* output = 
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->
    GetCompositeOutputData(port);
  return vtkMultiBlockDataSet::SafeDownCast(output);
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataSetAlgorithm::SetInput(vtkDataObject* input)
{
  this->SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataSetAlgorithm::SetInput(int index, vtkDataObject* input)
{
  if(input)
    {
    this->SetInputConnection(index, input->GetProducerPort());
    }
  else
    {
    // Setting a NULL input removes the connection.
    this->SetInputConnection(index, 0);
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkMultiBlockDataSetAlgorithm::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
    {
    return 0;
    }
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
int vtkMultiBlockDataSetAlgorithm::ProcessRequest(
  vtkInformation* request, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkCompositeDataPipeline::REQUEST_COMPOSITE_DATA()))
    {
    int retVal = this->RequestCompositeData(request, inputVector, outputVector);
    return retVal;
    }

  // execute information
  if(request->Has(vtkCompositeDataPipeline::REQUEST_COMPOSITE_INFORMATION()))
    {
    return this->RequestCompositeInformation(request, inputVector, outputVector);
    }

  // set update extent
 if(request->Has(
      vtkCompositeDataPipeline::REQUEST_COMPOSITE_UPDATE_EXTENT()))
    {
    return this->ComputeCompositeInputUpdateExtent(
      request, inputVector, outputVector);
    }

 return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkMultiBlockDataSetAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_TYPE_NAME(), 
            "vtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkMultiBlockDataSetAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkMultiBlockDataSetAlgorithm::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataSetAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
