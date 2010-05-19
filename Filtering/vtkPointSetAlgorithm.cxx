/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointSetAlgorithm.h"

#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkPointSetAlgorithm);

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
vtkPointSetAlgorithm::vtkPointSetAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPointSet* vtkPointSetAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkPointSet* vtkPointSetAlgorithm::GetOutput(int port)
{
  return vtkPointSet::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
// Get the output as vtkPolyData.
vtkPolyData *vtkPointSetAlgorithm::GetPolyDataOutput() 
{
  return vtkPolyData::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as vtkStructuredGrid.
vtkStructuredGrid *vtkPointSetAlgorithm::GetStructuredGridOutput()
{
  return vtkStructuredGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as vtkUnstructuredGrid.
vtkUnstructuredGrid *vtkPointSetAlgorithm::GetUnstructuredGridOutput()
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
void vtkPointSetAlgorithm::SetInput(vtkDataObject* input)
{
  this->SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkPointSetAlgorithm::SetInput(int index, vtkDataObject* input)
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
void vtkPointSetAlgorithm::SetInput(vtkPointSet* input)
{
  this->SetInput(0, static_cast<vtkDataObject*>(input));
}

//----------------------------------------------------------------------------
void vtkPointSetAlgorithm::SetInput(int index, vtkPointSet* input)
{
  this->SetInput(index, static_cast<vtkDataObject*>(input));
}

//----------------------------------------------------------------------------
void vtkPointSetAlgorithm::AddInput(vtkDataObject* input)
{
  this->AddInput(0, input);
}

//----------------------------------------------------------------------------
void vtkPointSetAlgorithm::AddInput(int index, vtkDataObject* input)
{
  if(input)
    {
    this->AddInputConnection(index, input->GetProducerPort());
    }
}

//----------------------------------------------------------------------------
void vtkPointSetAlgorithm::AddInput(vtkPointSet* input)
{
  this->AddInput(0, static_cast<vtkDataObject*>(input));
}

//----------------------------------------------------------------------------
void vtkPointSetAlgorithm::AddInput(int index, vtkPointSet* input)
{
  this->AddInput(index, static_cast<vtkDataObject*>(input));
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPointSetAlgorithm::GetInput()
{
  return this->GetExecutive()->GetInputData(0, 0);
}

//----------------------------------------------------------------------------
int vtkPointSetAlgorithm::ProcessRequest(
  vtkInformation* request, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);
    }

  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->ExecuteInformation(request, inputVector, outputVector);
    }

  // set update extent
 if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->ComputeInputUpdateExtent(request, inputVector, outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPointSetAlgorithm::RequestDataObject(
  vtkInformation*, 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkPointSet *input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkPointSet *output = vtkPointSet::SafeDownCast(
        info->Get(vtkDataObject::DATA_OBJECT()));
      
      if (!output || !output->IsA(input->GetClassName())) 
        {
        output = input->NewInstance();
        output->SetPipelineInformation(info);
        output->Delete();
        }
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPointSetAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPointSetAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPointSetAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
