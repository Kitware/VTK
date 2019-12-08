/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridAlgorithm.h"

#include "vtkBitArray.h"
#include "vtkCommand.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
vtkHyperTreeGridAlgorithm::vtkHyperTreeGridAlgorithm()
{
  // By default, only one input and one output ports
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  // Keep references to input and output data
  this->InData = nullptr; // todo: should be a safer pointer type
  this->OutData = nullptr;

  this->AppropriateOutput = false;
}

//----------------------------------------------------------------------------
vtkHyperTreeGridAlgorithm::~vtkHyperTreeGridAlgorithm()
{
  this->InData = nullptr;
  this->OutData = nullptr;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->InData)
  {
    os << indent << "InData:\n";
    this->InData->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "InData: ( none )\n";
  }

  os << indent << "OutData: ";
  if (this->OutData)
  {
    this->OutData->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkHyperTreeGridAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkHyperTreeGridAlgorithm::GetOutput(int port)
{
  return this->GetOutputDataObject(port);
}

//----------------------------------------------------------------------------
vtkHyperTreeGrid* vtkHyperTreeGridAlgorithm::GetHyperTreeGridOutput()
{
  return this->GetHyperTreeGridOutput(0);
}

//----------------------------------------------------------------------------
vtkHyperTreeGrid* vtkHyperTreeGridAlgorithm::GetHyperTreeGridOutput(int port)
{
  return vtkHyperTreeGrid::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
vtkPolyData* vtkHyperTreeGridAlgorithm::GetPolyDataOutput()
{
  return this->GetPolyDataOutput(0);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkHyperTreeGridAlgorithm::GetPolyDataOutput(int port)
{
  return vtkPolyData::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkHyperTreeGridAlgorithm::GetUnstructuredGridOutput()
{
  return this->GetUnstructuredGridOutput(0);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkHyperTreeGridAlgorithm::GetUnstructuredGridOutput(int port)
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAlgorithm::SetOutput(vtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridAlgorithm::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->GetNumberOfInputPorts() == 0 || this->GetNumberOfOutputPorts() == 0)
  {
    return 1;
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        vtkDataObject* newOutput = input->NewInstance();
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
vtkTypeBool vtkHyperTreeGridAlgorithm::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->AppropriateOutput)
  {
    // create the output
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
      return this->RequestDataObject(request, inputVector, outputVector);
    }
  }

  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridAlgorithm::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridAlgorithm::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridAlgorithm::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  // Do nothing and let subclasses handle it if needed
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridAlgorithm::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  int numInputPorts = this->GetNumberOfInputPorts();
  for (int i = 0; i < numInputPorts; ++i)
  {
    int numInputConnections = this->GetNumberOfInputConnections(i);
    for (int j = 0; j < numInputConnections; ++j)
    {
      vtkInformation* inputInfo = inputVector[i]->GetInformationObject(j);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridAlgorithm::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Update progress
  this->UpdateProgress(0.);

  // Retrieve input and output
  vtkHyperTreeGrid* input = vtkHyperTreeGrid::GetData(inputVector[0], 0);
  if (!input)
  {
    vtkErrorMacro("No input available. Cannot proceed with hyper tree grid algorithm.");
    return 0;
  }
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);
  if (!outputDO)
  {
    vtkErrorMacro("No output available. Cannot proceed with hyper tree grid algorithm.");
    return 0;
  }

  this->OutData = nullptr; // JB Pourquoi mettre au niveau de Algorithm le OutData ?

  // Process all trees in input grid and generate input data object
  // only if extents are correct
  if ((input->GetExtent()[0] <= input->GetExtent()[1] ||
        input->GetExtent()[2] <= input->GetExtent()[3] ||
        input->GetExtent()[4] <= input->GetExtent()[5]) &&
    !this->ProcessTrees(input, outputDO))
  {
    return 0;
  }

  // Squeeze output data if present
  if (this->OutData)
  {
    this->OutData->Squeeze();
  }

  // Update progress and return
  this->UpdateProgress(1.);
  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAlgorithm::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAlgorithm::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAlgorithm::AddInputData(vtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAlgorithm::AddInputData(int index, vtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}
