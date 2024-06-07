// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridAlgorithm.h"

#include "vtkCellGrid.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridAlgorithm);

//------------------------------------------------------------------------------
vtkCellGridAlgorithm::vtkCellGridAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkCellGridAlgorithm::~vtkCellGridAlgorithm() = default;

//------------------------------------------------------------------------------
void vtkCellGridAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkCellGrid* vtkCellGridAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkCellGrid* vtkCellGridAlgorithm::GetOutput(int port)
{
  return vtkCellGrid::SafeDownCast(this->GetOutputDataObject(port));
}

//------------------------------------------------------------------------------
void vtkCellGridAlgorithm::SetOutput(vtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkCellGridAlgorithm::GetInput()
{
  return this->GetInput(0);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkCellGridAlgorithm::GetInput(int port)
{
  return this->GetExecutive()->GetInputData(port, 0);
}

//------------------------------------------------------------------------------
vtkCellGrid* vtkCellGridAlgorithm::GetPolyDataInput(int port)
{
  return vtkCellGrid::SafeDownCast(this->GetInput(port));
}

//------------------------------------------------------------------------------
vtkTypeBool vtkCellGridAlgorithm::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_TIME()))
  {
    return this->RequestUpdateTime(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkCellGridAlgorithm::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkCellGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkCellGridAlgorithm::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCellGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkCellGridAlgorithm::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
  return 1;
}

//------------------------------------------------------------------------------
int vtkCellGridAlgorithm::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  int numInputPorts = this->GetNumberOfInputPorts();
  for (int i = 0; i < numInputPorts; i++)
  {
    int numInputConnections = this->GetNumberOfInputConnections(i);
    for (int j = 0; j < numInputConnections; j++)
    {
      vtkInformation* inputInfo = inputVector[i]->GetInformationObject(j);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkCellGridAlgorithm::RequestUpdateTime(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  return 1;
}

//------------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int vtkCellGridAlgorithm::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  return 1;
}

//------------------------------------------------------------------------------
void vtkCellGridAlgorithm::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//------------------------------------------------------------------------------
void vtkCellGridAlgorithm::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//------------------------------------------------------------------------------
void vtkCellGridAlgorithm::AddInputData(vtkDataObject* input)
{
  this->AddInputData(0, input);
}

//------------------------------------------------------------------------------
void vtkCellGridAlgorithm::AddInputData(int index, vtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}

//------------------------------------------------------------------------------
void vtkCellGridAlgorithm::SetInputAttributeToProcess(
  int idx, int port, int connection, const char* name)
{
  this->SetInputArrayToProcess(idx, port, connection, vtkDataObject::FIELD_ASSOCIATION_CELLS, name);
}

//------------------------------------------------------------------------------
vtkCellAttribute* vtkCellGridAlgorithm::GetInputCellAttributeToProcess(
  int idx, int connection, vtkInformationVector** inputVector)
{
  int association = vtkDataObject::FIELD_ASSOCIATION_NONE;
  return this->GetInputCellAttributeToProcess(idx, connection, inputVector, association);
}

//------------------------------------------------------------------------------
vtkCellAttribute* vtkCellGridAlgorithm::GetInputCellAttributeToProcess(
  int idx, int connection, vtkInformationVector** inputVector, int& association)
{
  vtkInformationVector* inArrayVec = this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
  if (!inArrayVec)
  {
    vtkErrorMacro("Attempt to get an input attribute for an index that has not been specified");
    return nullptr;
  }
  vtkInformation* inArrayInfo = inArrayVec->GetInformationObject(idx);
  if (!inArrayInfo)
  {
    vtkErrorMacro(
      "Attempt to get an input attribute for an index (" << idx << ") that has not been specified");
    return nullptr;
  }

  int port = inArrayInfo->Get(INPUT_PORT());
  vtkInformation* inInfo = inputVector[port]->GetInformationObject(connection);
  auto* input = vtkCellGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  return this->GetInputCellAttributeToProcess(idx, input, association);
}

//------------------------------------------------------------------------------
vtkCellAttribute* vtkCellGridAlgorithm::GetInputCellAttributeToProcess(int idx, vtkCellGrid* input)
{
  int association = vtkDataObject::FIELD_ASSOCIATION_NONE;
  return this->GetInputCellAttributeToProcess(idx, input, association);
}

//------------------------------------------------------------------------------
vtkCellAttribute* vtkCellGridAlgorithm::GetInputCellAttributeToProcess(
  int idx, vtkCellGrid* input, int& association)
{
  if (!input)
  {
    return nullptr;
  }

  vtkInformationVector* inArrayVec = this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
  if (!inArrayVec)
  {
    vtkErrorMacro("Attempt to get an input array for an index that has not been specified");
    return nullptr;
  }
  vtkInformation* inArrayInfo = inArrayVec->GetInformationObject(idx);
  if (!inArrayInfo)
  {
    vtkErrorMacro("Attempt to get an input array for an index that has not been specified");
    return nullptr;
  }

  int fieldAssoc = inArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());
  association = fieldAssoc;

  // For now, only FIELD_ASSOCIATION_CELLS can produce a vtkCellAttribute.
  // Requesting any other field association will yield a null pointer.
  if (fieldAssoc != vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    return nullptr;
  }

  if (inArrayInfo->Has(vtkDataObject::FIELD_NAME()))
  {
    const char* name = inArrayInfo->Get(vtkDataObject::FIELD_NAME());
    return input->GetCellAttributeByName(name);
  }
  else
  {
    return nullptr;
  }
}

VTK_ABI_NAMESPACE_END
