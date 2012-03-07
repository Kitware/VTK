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
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* vtkNotUsed( outputVector ))
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkRectilinearGridAlgorithm::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkRectilinearGridAlgorithm::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void vtkRectilinearGridAlgorithm::AddInputData(vtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkRectilinearGridAlgorithm::AddInputData(int index, vtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}

