/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetAlgorithm.h"

#include "vtkCommand.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkTrivialProducer.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkDataSetAlgorithm, "1.2");
vtkStandardNewMacro(vtkDataSetAlgorithm);

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
vtkDataSetAlgorithm::vtkDataSetAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkDataSet* vtkDataSetAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkDataSet* vtkDataSetAlgorithm::GetOutput(int port)
{
  return vtkDataSet::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
// Get the output as vtkPolyData.
vtkPolyData *vtkDataSetAlgorithm::GetPolyDataOutput() 
{
  return vtkPolyData::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as vtkStructuredPoints.
vtkStructuredPoints *vtkDataSetAlgorithm::GetStructuredPointsOutput() 
{
  return vtkStructuredPoints::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as vtkStructuredGrid.
vtkStructuredGrid *vtkDataSetAlgorithm::GetStructuredGridOutput()
{
  return vtkStructuredGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as vtkUnstructuredGrid.
vtkUnstructuredGrid *vtkDataSetAlgorithm::GetUnstructuredGridOutput()
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as vtkRectilinearGrid. 
vtkRectilinearGrid *vtkDataSetAlgorithm::GetRectilinearGridOutput()
{
  return vtkRectilinearGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
void vtkDataSetAlgorithm::SetInput(vtkDataObject* input)
{
  this->SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkDataSetAlgorithm::SetInput(int index, vtkDataObject* input)
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
vtkDataObject* vtkDataSetAlgorithm::GetInput(int port)
{
  return this->GetExecutive()->GetInputData(this,port,0);
}

//----------------------------------------------------------------------------
int vtkDataSetAlgorithm::ProcessRequest(
  vtkInformation* request, 
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

    int retVal = this->RequestData(request, inputVector, outputVector);

    if(!this->AbortExecute)
      {
      this->UpdateProgress(1.0);
      }
    this->InvokeEvent(vtkCommand::EndEvent,NULL);

    // Mark the data as up-to-date. I think this should be done in the
    // executive
    output->DataHasBeenGenerated();
    return retVal;
    }

  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->CreateOutput(request, inputVector, outputVector);
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->ExecuteInformation(request, inputVector, outputVector);
    }

  // set update extent
 if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->ComputeInputUpdateExtent(request, inputVector, outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkDataSetAlgorithm::CreateOutput(
  vtkInformation*, 
  vtkInformationVector* inputVector , 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = 
    inputVector->GetInformationObject(0)->Get(
      vtkAlgorithm::INPUT_CONNECTION_INFORMATION())->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  if (input)
    {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkDataSet *output = vtkDataSet::SafeDownCast(
      info->Get(vtkDataObject::DATA_OBJECT()));
    
    if (!output || !output->IsA(input->GetClassName())) 
      {
      output = input->NewInstance();
      this->GetExecutive()->SetOutputData(this, 0, output);
      info->Set(vtkDataObject::DATA_OBJECT(), output);
      output->Delete();
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkDataSetAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkDataSetAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataSetAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
