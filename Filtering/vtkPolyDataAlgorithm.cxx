/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataAlgorithm.h"

#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"

vtkCxxRevisionMacro(vtkPolyDataAlgorithm, "1.1.2.5");
vtkStandardNewMacro(vtkPolyDataAlgorithm);

//----------------------------------------------------------------------------
vtkPolyDataAlgorithm::vtkPolyDataAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPolyDataAlgorithm::~vtkPolyDataAlgorithm()
{
}

//----------------------------------------------------------------------------
void vtkPolyDataAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkPolyDataAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkPolyDataAlgorithm::GetOutput(int port)
{
  return vtkPolyData::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPolyDataAlgorithm::GetInput(int port)
{
  return this->GetExecutive()->GetInputData(this,port,0);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkPolyDataAlgorithm::GetPolyDataInput(int port)
{
  return vtkPolyData::SafeDownCast(this->GetInput(port));
}

//----------------------------------------------------------------------------
void vtkPolyDataAlgorithm::SetInput(vtkDataObject* input)
{
  this->SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkPolyDataAlgorithm::SetInput(int index, vtkDataObject* input)
{
  if(input)
    {
    if(vtkAlgorithmOutput* producerPort = input->GetProducerPort())
      {
      this->SetInputConnection(index, producerPort);
      }
    else
      {
      // The data object has no producer.  Give it a trivial producer.
      vtkTrivialProducer* producer = vtkTrivialProducer::New();
      producer->SetOutput(input);
      this->SetInputConnection(index, producer->GetOutputPort(0));
      producer->Delete();
      }
    }
  else
    {
    // Setting a NULL input removes the connection.
    this->SetInputConnection(index, 0);
    }
}

//----------------------------------------------------------------------------
void vtkPolyDataAlgorithm::AddInput(vtkDataObject* input)
{
  this->AddInput(0, input);
}

//----------------------------------------------------------------------------
void vtkPolyDataAlgorithm::AddInput(int index, vtkDataObject* input)
{
  if(input)
    {
    if(vtkAlgorithmOutput* producerPort = input->GetProducerPort())
      {
      this->AddInputConnection(index, producerPort);
      }
    else
      {
      // The data object has no producer.  Give it a trivial producer.
      vtkTrivialProducer* producer = vtkTrivialProducer::New();
      producer->SetOutput(input);
      this->AddInputConnection(index, producer->GetOutputPort(0));
      producer->Delete();
      }
    }
}

//----------------------------------------------------------------------------
int vtkPolyDataAlgorithm::ProcessRequest(vtkInformation* request,
                                         vtkInformationVector* inputVector,
                                         vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    // get the output data object
    vtkInformation* info = outputVector->GetInformationObject(0);

    // do we need to prepare all outputs? I think this should be done in the
    // executive
    vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
    output->PrepareForNewData();

    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    this->AbortExecute = 0;
    this->Progress = 0.0;

    this->ExecuteData(request, inputVector, outputVector);

    if(!this->AbortExecute)
      {
      this->UpdateProgress(1.0);
      }
    this->InvokeEvent(vtkCommand::EndEvent,NULL);

    // Mark the data as up-to-date. I think this should be done in the
    // executive
    output->DataHasBeenGenerated();
    return 1;
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    this->ExecuteInformation(request, inputVector, outputVector);
    return 1;
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int vtkPolyDataAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

int vtkPolyDataAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

void vtkPolyDataAlgorithm::ExecuteInformation(
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector * vtkNotUsed(inputVector), 
  vtkInformationVector * vtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
}

//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
void vtkPolyDataAlgorithm::ExecuteData(
  vtkInformation *request, 
  vtkInformationVector * vtkNotUsed( inputVector ), 
  vtkInformationVector *outputVector)
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
}

//----------------------------------------------------------------------------
// Assume that any source that implements ExecuteData 
// can handle an empty extent.
void vtkPolyDataAlgorithm::ExecuteData(vtkDataObject *output)
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
void vtkPolyDataAlgorithm::Execute()
{
  vtkErrorMacro(<< "Definition of Execute() method should be in subclass and you should really use the ExecuteData(vtkInformation *request,...) signature instead");
}

//----------------------------------------------------------------------------
int vtkPolyDataAlgorithm::UpdateExtentIsEmpty(vtkDataObject *output)
{
  if (output == NULL)
    {
    return 1;
    }

  int *ext = output->GetUpdateExtent();
  switch ( output->GetExtentType() )
    {
    case VTK_PIECES_EXTENT:
      // Special way of asking for no input.
      if ( output->GetUpdateNumberOfPieces() == 0 )
        {
        return 1;
        }
      break;

    case VTK_3D_EXTENT:
      // Special way of asking for no input. (zero volume)
      if (ext[0] == (ext[1] + 1) ||
          ext[2] == (ext[3] + 1) ||
          ext[4] == (ext[5] + 1))
      {
      return 1;
      }
      break;

    // We should never have this case occur
    default:
      vtkErrorMacro( << "Internal error - invalid extent type!" );
      break;
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPolyDataAlgorithm::ReleaseDataFlagOn()
{
  if(vtkDemandDrivenPipeline* ddp =
     vtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
    {
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      ddp->SetReleaseDataFlag(i, 1);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPolyDataAlgorithm::ReleaseDataFlagOff()
{
  if(vtkDemandDrivenPipeline* ddp =
     vtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
    {
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      ddp->SetReleaseDataFlag(i, 0);
      }
    }
}
