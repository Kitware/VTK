/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperOctreeAlgorithm.h"

#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkHyperOctree.h"
#include "vtkStreamingDemandDrivenPipeline.h"


//----------------------------------------------------------------------------
vtkHyperOctreeAlgorithm::vtkHyperOctreeAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkHyperOctreeAlgorithm::~vtkHyperOctreeAlgorithm()
{
}

//----------------------------------------------------------------------------
void vtkHyperOctreeAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkHyperOctree* vtkHyperOctreeAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkHyperOctree* vtkHyperOctreeAlgorithm::GetOutput(int port)
{
  return vtkHyperOctree::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void vtkHyperOctreeAlgorithm::SetOutput(vtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkHyperOctreeAlgorithm::GetInput()
{
  return this->GetInput(0);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkHyperOctreeAlgorithm::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
    {
    return 0;
    }
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
vtkHyperOctree* vtkHyperOctreeAlgorithm::GetHyperOctreeInput(int port)
{
  return vtkHyperOctree::SafeDownCast(this->GetInput(port));
}

//----------------------------------------------------------------------------
int vtkHyperOctreeAlgorithm::ProcessRequest(vtkInformation* request,
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
int vtkHyperOctreeAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperOctree");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperOctreeAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperOctree");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperOctreeAlgorithm::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperOctreeAlgorithm::RequestUpdateExtent(
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
int vtkHyperOctreeAlgorithm::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* vtkNotUsed( outputVector ) )
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkHyperOctreeAlgorithm::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkHyperOctreeAlgorithm::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void vtkHyperOctreeAlgorithm::AddInputData(vtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkHyperOctreeAlgorithm::AddInputData(int index, vtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}
