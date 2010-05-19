/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRectilinearGridAlgorithm.h"

#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"

vtkStandardNewMacro(vtkRectilinearGridAlgorithm);

//----------------------------------------------------------------------------
vtkRectilinearGridAlgorithm::vtkRectilinearGridAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkRectilinearGridAlgorithm::~vtkRectilinearGridAlgorithm()
{
}

//----------------------------------------------------------------------------
void vtkRectilinearGridAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkRectilinearGridAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkRectilinearGridAlgorithm::GetOutput(int port)
{
  return vtkRectilinearGrid::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void vtkRectilinearGridAlgorithm::SetOutput(vtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkRectilinearGridAlgorithm::GetInput()
{
  return this->GetInput(0);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkRectilinearGridAlgorithm::GetInput(int port)
{
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkRectilinearGridAlgorithm::GetRectilinearGridInput(int port)
{
  return vtkRectilinearGrid::SafeDownCast(this->GetInput(port));
}

//----------------------------------------------------------------------------
int vtkRectilinearGridAlgorithm::ProcessRequest(vtkInformation* request,
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
int vtkRectilinearGridAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkRectilinearGridAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkRectilinearGridAlgorithm::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
  return 1;
}

//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int vtkRectilinearGridAlgorithm::RequestData(
  vtkInformation* request,
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector)
{
  // the default implimentation is to do what the old pipeline did find what
  // output is requesting the data, and pass that into ExecuteData

  // which output port did the request come from
  int outputPort = 
    request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());

  // if output port is negative then that means this filter is calling the
  // update directly, in that case just assume port 0
  if (outputPort == -1)
      {
      outputPort = 0;
      }
  
  // get the data object
  vtkInformation *outInfo = 
    outputVector->GetInformationObject(outputPort);
  // call ExecuteData
  this->ExecuteData( outInfo->Get(vtkDataObject::DATA_OBJECT()) );

  return 1;
}

//----------------------------------------------------------------------------
// Assume that any source that implements ExecuteData 
// can handle an empty extent.
void vtkRectilinearGridAlgorithm::ExecuteData(vtkDataObject *output)
{
  // I want to find out if the requested extent is empty.
  if (output && this->UpdateExtentIsEmpty(output))
    {
    output->Initialize();
    return;
    }
  
  this->Execute();
}

//----------------------------------------------------------------------------
void vtkRectilinearGridAlgorithm::Execute()
{
  vtkErrorMacro(<< "Definition of Execute() method should be in subclass and you should really use the ExecuteData(vtkInformation *request,...) signature instead");
}

//----------------------------------------------------------------------------
void vtkRectilinearGridAlgorithm::SetInput(vtkDataObject* input)
{
  this->SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkRectilinearGridAlgorithm::SetInput(int index, vtkDataObject* input)
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
void vtkRectilinearGridAlgorithm::AddInput(vtkDataObject* input)
{
  this->AddInput(0, input);
}

//----------------------------------------------------------------------------
void vtkRectilinearGridAlgorithm::AddInput(int index, vtkDataObject* input)
{
  if(input)
    {
    this->AddInputConnection(index, input->GetProducerPort());
    }
}

