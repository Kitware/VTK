/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunctionAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPiecewiseFunctionAlgorithm.h"

#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkDataObject.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"

vtkStandardNewMacro(vtkPiecewiseFunctionAlgorithm);

//----------------------------------------------------------------------------
vtkPiecewiseFunctionAlgorithm::vtkPiecewiseFunctionAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPiecewiseFunctionAlgorithm::~vtkPiecewiseFunctionAlgorithm()
{
}

//----------------------------------------------------------------------------
void vtkPiecewiseFunctionAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPiecewiseFunctionAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPiecewiseFunctionAlgorithm::GetOutput(int port)
{
  return this->GetOutputDataObject(port);
}

//----------------------------------------------------------------------------
void vtkPiecewiseFunctionAlgorithm::SetOutput(vtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPiecewiseFunctionAlgorithm::GetInput()
{
  return this->GetInput(0);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPiecewiseFunctionAlgorithm::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
    {
    return 0;
    }
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
int vtkPiecewiseFunctionAlgorithm::ProcessRequest(vtkInformation* request,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPiecewiseFunctionAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPiecewiseFunction");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPiecewiseFunctionAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPiecewiseFunction");
  return 1;
}

//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int vtkPiecewiseFunctionAlgorithm::RequestData(
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
void vtkPiecewiseFunctionAlgorithm::ExecuteData(vtkDataObject *output)
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
void vtkPiecewiseFunctionAlgorithm::Execute()
{
  vtkErrorMacro(<< "Definition of Execute() method should be in subclass and you should really use the ExecuteData(vtkInformation *request,...) signature instead");
}

//----------------------------------------------------------------------------
int vtkPiecewiseFunctionAlgorithm::UpdateExtentIsEmpty(vtkDataObject *output)
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
void vtkPiecewiseFunctionAlgorithm::SetInput(vtkDataObject* input)
{
  this->SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkPiecewiseFunctionAlgorithm::SetInput(int index, vtkDataObject* input)
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
void vtkPiecewiseFunctionAlgorithm::AddInput(vtkDataObject* input)
{
  this->AddInput(0, input);
}

//----------------------------------------------------------------------------
void vtkPiecewiseFunctionAlgorithm::AddInput(int index, vtkDataObject* input)
{
  if(input)
    {
    this->AddInputConnection(index, input->GetProducerPort());
    }
}
