/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCompositeDataSetAlgorithm.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataSetAlgorithm.h"

#include "vtkCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkCompositeDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkCompositeDataSetAlgorithm);

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
vtkCompositeDataSetAlgorithm::vtkCompositeDataSetAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataSetAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataSetAlgorithm::GetOutput(int port)
{
  vtkDataObject* output = 
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->
    GetCompositeOutputData(port);
  return vtkCompositeDataSet::SafeDownCast(output);
}

//----------------------------------------------------------------------------
void vtkCompositeDataSetAlgorithm::SetInput(vtkDataObject* input)
{
  this->SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkCompositeDataSetAlgorithm::SetInput(int index, vtkDataObject* input)
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
vtkDataObject* vtkCompositeDataSetAlgorithm::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
    {
    return 0;
    }
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
int vtkCompositeDataSetAlgorithm::ProcessRequest(
  vtkInformation* request, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }

  // generate the data
  if(request->Has(vtkCompositeDataPipeline::REQUEST_DATA()))
    {
    int retVal = this->RequestData(request, inputVector, outputVector);
    return retVal;
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    if(request->Has(vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT()))
      {
      int outputPort = request->Get(
        vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT());
      vtkInformation* info = outputVector->GetInformationObject(outputPort);
      if (info)
        {
        info->Set(
          vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);
        }
      }
    else
      {
      for (int outIdx=0; outIdx < this->GetNumberOfOutputPorts(); outIdx++)
        {
        vtkInformation* info = outputVector->GetInformationObject(outIdx);
        if (info)
          {
          info->Set(
            vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);
          }
        }
      }
    return this->RequestInformation(request, inputVector, outputVector);
    }

  // set update extent
  if(request->Has(
       vtkCompositeDataPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkCompositeDataSetAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkCompositeDataSetAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkCompositeDataSetAlgorithm::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void vtkCompositeDataSetAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
