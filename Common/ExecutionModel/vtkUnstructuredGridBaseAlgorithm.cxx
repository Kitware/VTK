/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridBaseAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridBaseAlgorithm.h"

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkUnstructuredGridBase.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkUnstructuredGridBaseAlgorithm)

//----------------------------------------------------------------------------
vtkUnstructuredGridBaseAlgorithm::vtkUnstructuredGridBaseAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkUnstructuredGridBaseAlgorithm::~vtkUnstructuredGridBaseAlgorithm()
{
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridBaseAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkUnstructuredGridBase *vtkUnstructuredGridBaseAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkUnstructuredGridBase *vtkUnstructuredGridBaseAlgorithm::GetOutput(int port)
{
  return vtkUnstructuredGridBase::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridBaseAlgorithm::SetOutput(vtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridBaseAlgorithm::ProcessRequest(vtkInformation* request,
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

  // set update extent
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
int vtkUnstructuredGridBaseAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGridBase");
  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridBaseAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
            "vtkUnstructuredGridBase");
  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridBaseAlgorithm::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridBaseAlgorithm::RequestUpdateExtent(
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
int vtkUnstructuredGridBaseAlgorithm::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* vtkNotUsed( outputVector ))
{
  return 0;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridBaseAlgorithm::RequestDataObject(
    vtkInformation *, vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkUnstructuredGridBase *input = vtkUnstructuredGridBase::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input)
  {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkUnstructuredGridBase *output = vtkUnstructuredGridBase::SafeDownCast(
            info->Get(vtkDataObject::DATA_OBJECT()));

      if (!output || !output->IsA(input->GetClassName()))
      {
        vtkUnstructuredGridBase* newOutput = input->NewInstance();
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridBaseAlgorithm::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridBaseAlgorithm::SetInputData(int index,
                                                    vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridBaseAlgorithm::AddInputData(vtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridBaseAlgorithm::AddInputData(int index,
                                                    vtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}
