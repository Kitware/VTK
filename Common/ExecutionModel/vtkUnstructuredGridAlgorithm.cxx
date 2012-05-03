/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridAlgorithm.h"

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkUnstructuredGridAlgorithm);

//----------------------------------------------------------------------------
vtkUnstructuredGridAlgorithm::vtkUnstructuredGridAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkUnstructuredGridAlgorithm::~vtkUnstructuredGridAlgorithm()
{
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkUnstructuredGridAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkUnstructuredGridAlgorithm::GetOutput(int port)
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridAlgorithm::SetOutput(vtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkUnstructuredGridAlgorithm::GetInput(int port)
{
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkUnstructuredGridAlgorithm::GetUnstructuredGridInput(int port)
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetInput(port));
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridAlgorithm::ProcessRequest(vtkInformation* request,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);
    }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridAlgorithm::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridAlgorithm::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  int numInputPorts = this->GetNumberOfInputPorts();
  for (int i=0; i<numInputPorts; i++)
    {
    int numInputConnections = this->GetNumberOfInputConnections(i);
    for (int j=0; j<numInputConnections; j++)
      {
      vtkInformation* inputInfo = inputVector[i]->GetInformationObject(j);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int vtkUnstructuredGridAlgorithm::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* vtkNotUsed( outputVector ))
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridAlgorithm::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridAlgorithm::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridAlgorithm::AddInputData(vtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridAlgorithm::AddInputData(int index, vtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}

