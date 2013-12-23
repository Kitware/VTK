/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxDataSetAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalBoxDataSetAlgorithm.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkHierarchicalBoxDataSetAlgorithm);
//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSetAlgorithm::vtkHierarchicalBoxDataSetAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSetAlgorithm::~vtkHierarchicalBoxDataSetAlgorithm()
{
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* vtkHierarchicalBoxDataSetAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* vtkHierarchicalBoxDataSetAlgorithm::GetOutput(int port)
{
  vtkDataObject* output =
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->
    GetCompositeOutputData(port);
  return vtkHierarchicalBoxDataSet::SafeDownCast(output);
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSetAlgorithm::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSetAlgorithm::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkHierarchicalBoxDataSetAlgorithm::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
    {
    return 0;
    }
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSetAlgorithm::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }

  // generate the data
  if(request->Has(vtkCompositeDataPipeline::REQUEST_DATA()))
    {
    int retVal = this->RequestData(request, inputVector, outputVector);
    return retVal;
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);
    }

  // set update extent
  if(request->Has(
       vtkCompositeDataPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSetAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHierarchicalBoxDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSetAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalBoxDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkHierarchicalBoxDataSetAlgorithm::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSetAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

