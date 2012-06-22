/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPistonAlgorithm.h"

#include "vtkCommand.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPistonDataObject.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPistonAlgorithm);

//----------------------------------------------------------------------------
vtkPistonAlgorithm::vtkPistonAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                             vtkDataSetAttributes::SCALARS);

}

//----------------------------------------------------------------------------
vtkPistonAlgorithm::~vtkPistonAlgorithm()
{
}

//----------------------------------------------------------------------------
void vtkPistonAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPistonAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPistonDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPistonAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPistonDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPistonAlgorithm::ProcessRequest(vtkInformation* request,
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

  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}


//----------------------------------------------------------------------------
int vtkPistonAlgorithm::RequestDataObject(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
  vtkPistonDataObject *newobj = vtkPistonDataObject::New();
  this->GetExecutive()->SetOutputData(0, newobj);
  newobj->Delete();
  return 1;
}

//----------------------------------------------------------------------------
int vtkPistonAlgorithm::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
  return 1;
}

//----------------------------------------------------------------------------
int vtkPistonAlgorithm::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  //TODO: this may need some work
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
int vtkPistonAlgorithm::RequestData(
  vtkInformation* request,
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector)
{
  // do nothing let subclasses handle it
  return 1;
}

//----------------------------------------------------------------------------
void vtkPistonAlgorithm::PassBoundsForward
  (vtkPistonDataObject *id, vtkPistonDataObject *od)
{
  od->SetBounds(id->GetBounds());
  od->SetOrigin(id->GetOrigin());
  od->SetSpacing(id->GetSpacing());
}

//----------------------------------------------------------------------------
void vtkPistonAlgorithm::SetInputData(int idx, vtkDataObject *input)
{
  this->SetInputDataInternal(idx, input);
}

//----------------------------------------------------------------------------
vtkPistonDataObject* vtkPistonAlgorithm::GetPistonDataObjectOutput(int port)
{
  return vtkPistonDataObject::SafeDownCast(this->GetOutputDataObject(port));
}
