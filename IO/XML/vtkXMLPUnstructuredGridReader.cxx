// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLPUnstructuredGridReader.h"

#include "vtkAbstractArray.h"
#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLUnstructuredGridReader.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLPUnstructuredGridReader);

//------------------------------------------------------------------------------
vtkXMLPUnstructuredGridReader::vtkXMLPUnstructuredGridReader() = default;

//------------------------------------------------------------------------------
vtkXMLPUnstructuredGridReader::~vtkXMLPUnstructuredGridReader() = default;

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLPUnstructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLPUnstructuredGridReader::GetOutput(int idx)
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//------------------------------------------------------------------------------
const char* vtkXMLPUnstructuredGridReader::GetDataSetName()
{
  return "PUnstructuredGrid";
}

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::GetOutputUpdateExtent(
  int& piece, int& numberOfPieces, int& ghostLevel)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::SetupNextPiece()
{
  this->Superclass::SetupNextPiece();
  if (this->PieceReaders[this->Piece])
  {
    this->StartCell += this->PieceReaders[this->Piece]->GetNumberOfCells();
  }
}

//------------------------------------------------------------------------------
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

  // Copy the polyhedra after the cells of the previous pieces, with their point ids offset, so
  // that the face locations stay aligned with the cells: the cells of the previous pieces, and of
  // this piece, without polyhedra have no face.
  vtkPolyhedronCellArray* inputPolyhedra = input->GetPolyhedronCellArray();
  if (inputPolyhedra && !output->GetPolyhedronCellArray())
  {
    output->InitializeFacesRepresentation(this->StartCell);
  }
  if (vtkPolyhedronCellArray* outputPolyhedra = output->GetPolyhedronCellArray())
  {
    if (inputPolyhedra)
    {
      outputPolyhedra->Append(inputPolyhedra, this->StartPoint);
    }
    else
    {
      outputPolyhedra->InsertEmptyCells(input->GetNumberOfCells());
    }
  }

  // Copy the corresponding cell types.
  auto inTypes = input->GetCellTypes();
  auto outTypes = output->GetCellTypes();
  outTypes->InsertTuples(this->StartCell, inTypes->GetNumberOfTuples(), 0, inTypes);

  return 1;
}

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::CopyArrayForCells(
  vtkAbstractArray* inArray, vtkAbstractArray* outArray)
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
  outArray->InsertTuples(this->StartCell, numCells, 0, inArray);
}

//------------------------------------------------------------------------------
vtkXMLDataReader* vtkXMLPUnstructuredGridReader::CreatePieceReader()
{
  return vtkXMLUnstructuredGridReader::New();
}

//------------------------------------------------------------------------------
int vtkXMLPUnstructuredGridReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::SqueezeOutputArrays(vtkDataObject* output)
{
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(output);
  if (vtkPolyhedronCellArray* outputPolyhedra = grid->GetPolyhedronCellArray())
  {
    outputPolyhedra->Squeeze();
  }
}
VTK_ABI_NAMESPACE_END
