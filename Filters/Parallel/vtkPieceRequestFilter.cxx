/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieceRequestFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPieceRequestFilter.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPieceRequestFilter);

//----------------------------------------------------------------------------
vtkPieceRequestFilter::vtkPieceRequestFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->Piece = 0;
  this->NumberOfPieces = 1;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPieceRequestFilter::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPieceRequestFilter::GetOutput(int port)
{
  return vtkDataObject::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void vtkPieceRequestFilter::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkPieceRequestFilter::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
int vtkPieceRequestFilter::ProcessRequest(
  vtkInformation* request,
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
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPieceRequestFilter::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (input)
  {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        vtkDataObject* newOutput = input->NewInstance();
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPieceRequestFilter::RequestData(vtkInformation* vtkNotUsed(request),
                                       vtkInformationVector** inputVector,
                                       vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkDataObject* output = vtkDataObject::GetData(outputVector);
  output->ShallowCopy(input);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPieceRequestFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
                                               vtkInformationVector** inputVector,
                                               vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), this->NumberOfPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), this->Piece);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPieceRequestFilter::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPieceRequestFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPieceRequestFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
  os << indent << "Piece: " << this->Piece << endl;
}
