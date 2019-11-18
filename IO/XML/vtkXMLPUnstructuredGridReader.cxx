/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPUnstructuredGridReader.h"

#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLUnstructuredGridReader.h"

vtkStandardNewMacro(vtkXMLPUnstructuredGridReader);

//----------------------------------------------------------------------------
vtkXMLPUnstructuredGridReader::vtkXMLPUnstructuredGridReader() = default;

//----------------------------------------------------------------------------
vtkXMLPUnstructuredGridReader::~vtkXMLPUnstructuredGridReader() = default;

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLPUnstructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLPUnstructuredGridReader::GetOutput(int idx)
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* vtkXMLPUnstructuredGridReader::GetDataSetName()
{
  return "PUnstructuredGrid";
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::GetOutputUpdateExtent(
  int& piece, int& numberOfPieces, int& ghostLevel)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::SetupOutputTotals()
{
  this->Superclass::SetupOutputTotals();
  // Find the total size of the output.
  this->TotalNumberOfCells = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    if (this->PieceReaders[i])
    {
      this->TotalNumberOfCells += this->PieceReaders[i]->GetNumberOfCells();
    }
  }

  // Data reading will start at the beginning of the output.
  this->StartCell = 0;
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(this->GetCurrentOutput());

  // Setup the output's cell arrays.
  vtkUnsignedCharArray* cellTypes = vtkUnsignedCharArray::New();
  cellTypes->SetNumberOfTuples(this->GetNumberOfCells());
  vtkCellArray* outCells = vtkCellArray::New();

  output->SetCells(cellTypes, outCells);

  outCells->Delete();
  cellTypes->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::SetupNextPiece()
{
  this->Superclass::SetupNextPiece();
  if (this->PieceReaders[this->Piece])
  {
    this->StartCell += this->PieceReaders[this->Piece]->GetNumberOfCells();
  }
}

//----------------------------------------------------------------------------
int vtkXMLPUnstructuredGridReader::ReadPieceData()
{
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  vtkPointSet* ips = this->GetPieceInputAsPointSet(this->Piece);
  vtkUnstructuredGrid* input = static_cast<vtkUnstructuredGrid*>(ips);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(this->GetCurrentOutput());

  // Copy the Cells.
  this->CopyCellArray(this->TotalNumberOfCells, input->GetCells(), output->GetCells());

  // Copy Faces and FaceLocations with offset adjustment if they exist
  if (vtkIdTypeArray* inputFaces = input->GetFaces())
  {
    vtkIdTypeArray* inputFaceLocations = input->GetFaceLocations();
    vtkIdTypeArray* outputFaces = output->GetFaces();
    if (!outputFaces)
    {
      output->InitializeFacesRepresentation(0);
      outputFaces = output->GetFaces();
    }
    vtkIdTypeArray* outputFaceLocations = output->GetFaceLocations();
    const vtkIdType numFaceLocs = inputFaceLocations->GetNumberOfValues();
    for (vtkIdType i = 0; i < numFaceLocs; ++i)
    {
      outputFaceLocations->InsertNextValue(outputFaces->GetMaxId() + 1);
      vtkIdType location = inputFaceLocations->GetValue(i);
      if (location < 0) // the face offsets array contains -1 for regular cells
      {
        continue;
      }

      vtkIdType numFaces = inputFaces->GetValue(location);
      location++;
      outputFaces->InsertNextValue(numFaces);
      for (vtkIdType f = 0; f < numFaces; f++)
      {
        vtkIdType numPoints = inputFaces->GetValue(location);
        outputFaces->InsertNextValue(numPoints);
        location++;
        for (vtkIdType p = 0; p < numPoints; p++)
        {
          // only the point ids get the offset
          outputFaces->InsertNextValue(inputFaces->GetValue(location) + this->StartPoint);
          location++;
        }
      }
    }
  }

  // Copy the corresponding cell types.
  vtkUnsignedCharArray* inTypes = input->GetCellTypesArray();
  vtkUnsignedCharArray* outTypes = output->GetCellTypesArray();
  vtkIdType components = outTypes->GetNumberOfComponents();
  memcpy(outTypes->GetVoidPointer(this->StartCell * components), inTypes->GetVoidPointer(0),
    inTypes->GetNumberOfTuples() * components * inTypes->GetDataTypeSize());

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::CopyArrayForCells(vtkDataArray* inArray, vtkDataArray* outArray)
{
  if (!this->PieceReaders[this->Piece])
  {
    return;
  }
  if (!inArray || !outArray)
  {
    return;
  }

  vtkIdType numCells = this->PieceReaders[this->Piece]->GetNumberOfCells();
  vtkIdType components = outArray->GetNumberOfComponents();
  vtkIdType tupleSize = inArray->GetDataTypeSize() * components;
  memcpy(outArray->GetVoidPointer(this->StartCell * components), inArray->GetVoidPointer(0),
    numCells * tupleSize);
}

//----------------------------------------------------------------------------
vtkXMLDataReader* vtkXMLPUnstructuredGridReader::CreatePieceReader()
{
  return vtkXMLUnstructuredGridReader::New();
}

//----------------------------------------------------------------------------
int vtkXMLPUnstructuredGridReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::SqueezeOutputArrays(vtkDataObject* output)
{
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(output);
  if (vtkIdTypeArray* outputFaces = grid->GetFaces())
  {
    outputFaces->Squeeze();
  }
  if (vtkIdTypeArray* outputFaceLocations = grid->GetFaceLocations())
  {
    outputFaceLocations->Squeeze();
  }
}
