/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataSetAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalDataSetAlgorithm.h"

#include "vtkCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkHierarchicalDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkHierarchicalDataSetAlgorithm, "1.1");
vtkStandardNewMacro(vtkHierarchicalDataSetAlgorithm);

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
vtkHierarchicalDataSetAlgorithm::vtkHierarchicalDataSetAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkHierarchicalDataSet* vtkHierarchicalDataSetAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkHierarchicalDataSet* vtkHierarchicalDataSetAlgorithm::GetOutput(int port)
{
  vtkDataObject* output = 
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->
    GetCompositeOutputData(port);
  return vtkHierarchicalDataSet::SafeDownCast(output);
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSetAlgorithm::SetInput(vtkDataObject* input)
{
  this->SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSetAlgorithm::SetInput(int index, vtkDataObject* input)
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
vtkDataObject* vtkHierarchicalDataSetAlgorithm::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
    {
    return 0;
    }
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
int vtkHierarchicalDataSetAlgorithm::ProcessRequest(
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
int vtkHierarchicalDataSetAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_TYPE_NAME(), 
            "vtkHierarchicalDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHierarchicalDataSetAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkHierarchicalDataSetAlgorithm::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSetAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
