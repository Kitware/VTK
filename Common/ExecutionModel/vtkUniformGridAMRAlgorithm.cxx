/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkUniformGridAMRAlgorithm.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkUniformGridAMRAlgorithm.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGridAMR.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkExecutive.h"

vtkStandardNewMacro(vtkUniformGridAMRAlgorithm);

//------------------------------------------------------------------------------
vtkUniformGridAMRAlgorithm::vtkUniformGridAMRAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkUniformGridAMRAlgorithm::~vtkUniformGridAMRAlgorithm()
{

}

//------------------------------------------------------------------------------
void vtkUniformGridAMRAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
vtkUniformGridAMR* vtkUniformGridAMRAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkUniformGridAMR* vtkUniformGridAMRAlgorithm::GetOutput(int port)
{
  vtkDataObject* output =
      vtkCompositeDataPipeline::SafeDownCast(
          this->GetExecutive())->GetCompositeOutputData(port);
  return( vtkUniformGridAMR::SafeDownCast(output) );
}

//------------------------------------------------------------------------------
void vtkUniformGridAMRAlgorithm::SetInputData( vtkDataObject* input )
{
  this->SetInputData(0, input);
}

//------------------------------------------------------------------------------
void vtkUniformGridAMRAlgorithm::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index,input);
}

//------------------------------------------------------------------------------
int vtkUniformGridAMRAlgorithm::ProcessRequest(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector )
{
  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }

  // generate the data
  if(request->Has(vtkCompositeDataPipeline::REQUEST_DATA()))
    {
    int retVal = this->RequestData(request,inputVector,outputVector);
    return( retVal );
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);
    }

  // set update extent
  if( request->Has(
        vtkCompositeDataPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return( this->RequestUpdateExtent(request,inputVector,outputVector) );
    }


  return( this->Superclass::ProcessRequest(request,inputVector,outputVector) );
}

//------------------------------------------------------------------------------
vtkExecutive* vtkUniformGridAMRAlgorithm::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//------------------------------------------------------------------------------
int vtkUniformGridAMRAlgorithm::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkUniformGridAMR");
  return 1;
}

//------------------------------------------------------------------------------
int vtkUniformGridAMRAlgorithm::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkUniformGridAMR");
  return 1;
}

//------------------------------------------------------------------------------
vtkDataObject* vtkUniformGridAMRAlgorithm::GetInput(int port)
{
  if( this->GetNumberOfInputConnections(port) < 1 )
    {
    return NULL;
    }
  return this->GetExecutive()->GetInputData(port,0);
}
