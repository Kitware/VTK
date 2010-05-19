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
  this->NumberOfStreamDivisions = 2;
  this->ColorByPiece = 0;
}

//----------------------------------------------------------------------------
vtkPolyDataStreamer::~vtkPolyDataStreamer()
{
}

//----------------------------------------------------------------------------
void vtkPolyDataStreamer::SetNumberOfStreamDivisions(int num)
{
  if (this->NumberOfStreamDivisions == num)
    {
    return;
    }

  this->Modified();
  this->NumberOfStreamDivisions = num;
}

//----------------------------------------------------------------------------
int vtkPolyDataStreamer::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info object
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // If we are actually streaming, then bypass the normal update process.
  if (this->NumberOfStreamDivisions > 1)
    {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), -1);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
    }

  return 1;
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int vtkPolyDataStreamer::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPolyData *copy;
  vtkAppendPolyData *append = vtkAppendPolyData::New();
  int outPiece, outNumPieces, outGhost;
  int i, j, inPiece;
  vtkFloatArray *pieceColors = NULL;
  float tmp;

  if (this->ColorByPiece)
    {
    pieceColors = vtkFloatArray::New();
    }

  outGhost = output->GetUpdateGhostLevel();
  outPiece = output->GetUpdatePiece();
  outNumPieces = output->GetUpdateNumberOfPieces();
  for (i = 0; i < this->NumberOfStreamDivisions; ++i)
    {
    inPiece = outPiece * this->NumberOfStreamDivisions + i;
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                inPiece);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                outNumPieces *this->NumberOfStreamDivisions);
    input->Update();
    copy = vtkPolyData::New();
    copy->ShallowCopy(input);
    append->AddInput(copy);
    copy->Delete();
    if (pieceColors)
      {
      for (j = 0; j < input->GetNumberOfCells(); ++j)
        {
        tmp = static_cast<float>(inPiece);
        pieceColors->InsertNextTuple(&tmp);
        }
      }
    }

  append->Update();
  output->ShallowCopy(append->GetOutput());
  // set the piece and number of pieces back to the correct value
  // since the shallow copy of the append filter has overwritten them.
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
               outNumPieces);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
               outPiece);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
               outGhost);

  if (pieceColors)
    {
    int idx = output->GetCellData()->AddArray(pieceColors);
    output->GetCellData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    pieceColors->Delete();
    }
  append->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyDataStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfStreamDivisions: " << this->NumberOfStreamDivisions << endl;
  os << indent << "ColorByPiece: " << this->ColorByPiece << endl;
}
