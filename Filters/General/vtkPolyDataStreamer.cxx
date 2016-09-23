/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataStreamer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataStreamer.h"

#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPolyDataStreamer);

//----------------------------------------------------------------------------
vtkPolyDataStreamer::vtkPolyDataStreamer()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->NumberOfPasses = 2;
  this->ColorByPiece = 0;

  this->Append = vtkAppendPolyData::New();
}

//----------------------------------------------------------------------------
vtkPolyDataStreamer::~vtkPolyDataStreamer()
{
  this->Append->Delete();
  this->Append = 0;
}

//----------------------------------------------------------------------------
void vtkPolyDataStreamer::SetNumberOfStreamDivisions(int num)
{
  if (this->NumberOfPasses == (unsigned int)num)
  {
    return;
  }

  this->Modified();
  this->NumberOfPasses = num;
}

//----------------------------------------------------------------------------
int vtkPolyDataStreamer::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int outPiece = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int outNumPieces = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
              outPiece * this->NumberOfPasses + this->CurrentIndex);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
              outNumPieces * this->NumberOfPasses);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPolyDataStreamer::ExecutePass(
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPolyData *copy  = vtkPolyData::New();
  copy->ShallowCopy(input);
  this->Append->AddInputData(copy);

  if (this->ColorByPiece)
  {
    int inPiece = inInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    vtkFloatArray *pieceColors = vtkFloatArray::New();
    pieceColors->SetName("Piece Colors");
    vtkIdType numCells = input->GetNumberOfCells();
    pieceColors->SetNumberOfTuples(numCells);
    for (vtkIdType j = 0; j < numCells; ++j)
    {
      pieceColors->SetValue(j, inPiece);
    }
    int idx = copy->GetCellData()->AddArray(pieceColors);
    copy->GetCellData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    pieceColors->Delete();
  }

  copy->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkPolyDataStreamer::PostExecute(
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  this->Append->Update();
  output->ShallowCopy(this->Append->GetOutput());
  this->Append->RemoveAllInputConnections(0);
  this->Append->GetOutput()->Initialize();

  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyDataStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfStreamDivisions: " << this->NumberOfPasses << endl;
  os << indent << "ColorByPiece: " << this->ColorByPiece << endl;
}

//----------------------------------------------------------------------------
int vtkPolyDataStreamer::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPolyDataStreamer::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}
