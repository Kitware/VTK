/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPPolyDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPPolyDataReader.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkXMLPPolyDataReader);

//----------------------------------------------------------------------------
vtkXMLPPolyDataReader::vtkXMLPPolyDataReader()
{
}

//----------------------------------------------------------------------------
vtkXMLPPolyDataReader::~vtkXMLPPolyDataReader()
{
}

//----------------------------------------------------------------------------
void vtkXMLPPolyDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkXMLPPolyDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkXMLPPolyDataReader::GetOutput(int idx)
{
  return vtkPolyData::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* vtkXMLPPolyDataReader::GetDataSetName()
{
  return "PPolyData";
}

//----------------------------------------------------------------------------
void vtkXMLPPolyDataReader::GetOutputUpdateExtent(int& piece,
                                                  int& numberOfPieces,
                                                  int& ghostLevel)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces= outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPPolyDataReader::GetNumberOfCellsInPiece(int piece)
{
  if (this->PieceReaders[piece])
  {
    return this->PieceReaders[piece]->GetNumberOfCells();
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPPolyDataReader::GetNumberOfVertsInPiece(int piece)
{
  if (this->PieceReaders[piece])
  {
    vtkXMLPolyDataReader* pReader =
      static_cast<vtkXMLPolyDataReader*>(this->PieceReaders[piece]);
    return pReader->GetNumberOfVerts();
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPPolyDataReader::GetNumberOfLinesInPiece(int piece)
{
  if (this->PieceReaders[piece])
  {
    vtkXMLPolyDataReader* pReader =
      static_cast<vtkXMLPolyDataReader*>(this->PieceReaders[piece]);
    return pReader->GetNumberOfLines();
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPPolyDataReader::GetNumberOfStripsInPiece(int piece)
{
  if (this->PieceReaders[piece])
  {
    vtkXMLPolyDataReader* pReader =
      static_cast<vtkXMLPolyDataReader*>(this->PieceReaders[piece]);
    return pReader->GetNumberOfStrips();
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPPolyDataReader::GetNumberOfPolysInPiece(int piece)
{
  if (this->PieceReaders[piece])
  {
    vtkXMLPolyDataReader* pReader =
      static_cast<vtkXMLPolyDataReader*>(this->PieceReaders[piece]);
    return pReader->GetNumberOfPolys();
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkXMLPPolyDataReader::SetupOutputTotals()
{
  this->Superclass::SetupOutputTotals();
  // Find the total size of the output.
  this->TotalNumberOfCells = 0;
  this->TotalNumberOfVerts = 0;
  this->TotalNumberOfLines = 0;
  this->TotalNumberOfStrips = 0;
  this->TotalNumberOfPolys = 0;
  for(int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    this->TotalNumberOfCells += this->GetNumberOfCellsInPiece(i);
    this->TotalNumberOfVerts += this->GetNumberOfVertsInPiece(i);
    this->TotalNumberOfLines += this->GetNumberOfLinesInPiece(i);
    this->TotalNumberOfStrips += this->GetNumberOfStripsInPiece(i);
    this->TotalNumberOfPolys += this->GetNumberOfPolysInPiece(i);
  }

  // Data reading will start at the beginning of the output.
  this->StartVert = 0;
  this->StartLine = 0;
  this->StartStrip = 0;
  this->StartPoly = 0;
}

//----------------------------------------------------------------------------
void vtkXMLPPolyDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  vtkPolyData* output = vtkPolyData::SafeDownCast(this->GetCurrentOutput());

  // Setup the output's cell arrays.
  vtkCellArray* outVerts = vtkCellArray::New();
  vtkCellArray* outLines = vtkCellArray::New();
  vtkCellArray* outStrips = vtkCellArray::New();
  vtkCellArray* outPolys = vtkCellArray::New();

  output->SetVerts(outVerts);
  output->SetLines(outLines);
  output->SetStrips(outStrips);
  output->SetPolys(outPolys);

  outPolys->Delete();
  outStrips->Delete();
  outLines->Delete();
  outVerts->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLPPolyDataReader::SetupNextPiece()
{
  this->Superclass::SetupNextPiece();
  this->StartVert += this->GetNumberOfVertsInPiece(this->Piece);
  this->StartLine += this->GetNumberOfLinesInPiece(this->Piece);
  this->StartStrip += this->GetNumberOfStripsInPiece(this->Piece);
  this->StartPoly += this->GetNumberOfPolysInPiece(this->Piece);
}

//----------------------------------------------------------------------------
int vtkXMLPPolyDataReader::ReadPieceData()
{
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  vtkPointSet* ips = this->GetPieceInputAsPointSet(this->Piece);
  vtkPolyData* input = static_cast<vtkPolyData*>(ips);
  vtkPolyData* output = vtkPolyData::SafeDownCast(this->GetCurrentOutput());

  // Copy the Verts.
  this->CopyCellArray(this->TotalNumberOfVerts, input->GetVerts(),
                      output->GetVerts());

  // Copy the Lines.
  this->CopyCellArray(this->TotalNumberOfLines, input->GetLines(),
                      output->GetLines());

  // Copy the Strips.
  this->CopyCellArray(this->TotalNumberOfStrips, input->GetStrips(),
                      output->GetStrips());

  // Copy the Polys.
  this->CopyCellArray(this->TotalNumberOfPolys, input->GetPolys(),
                      output->GetPolys());

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPPolyDataReader::CopyArrayForCells(vtkDataArray* inArray,
                                              vtkDataArray* outArray)
{
  if (!this->PieceReaders[this->Piece])
  {
    return;
  }
  if (inArray == NULL || outArray == NULL)
  {
    return;
  }

  vtkIdType components = outArray->GetNumberOfComponents();
  vtkIdType tupleSize = inArray->GetDataTypeSize()*components;

  // Copy the cell data for the Verts in the piece.
  vtkIdType inStartCell = 0;
  vtkIdType outStartCell = this->StartVert;
  vtkIdType numCells = this->GetNumberOfVertsInPiece(this->Piece);
  memcpy(outArray->GetVoidPointer(outStartCell*components),
         inArray->GetVoidPointer(inStartCell*components), numCells*tupleSize);

  // Copy the cell data for the Lines in the piece.
  inStartCell += numCells;
  outStartCell = this->TotalNumberOfVerts + this->StartLine;
  numCells = this->GetNumberOfLinesInPiece(this->Piece);
  memcpy(outArray->GetVoidPointer(outStartCell*components),
         inArray->GetVoidPointer(inStartCell*components), numCells*tupleSize);

  // Copy the cell data for the Strips in the piece.
  inStartCell += numCells;
  outStartCell = (this->TotalNumberOfVerts + this->TotalNumberOfLines +
                  this->StartStrip);
  numCells = this->GetNumberOfStripsInPiece(this->Piece);
  memcpy(outArray->GetVoidPointer(outStartCell*components),
         inArray->GetVoidPointer(inStartCell*components), numCells*tupleSize);

  // Copy the cell data for the Polys in the piece.
  inStartCell += numCells;
  outStartCell = (this->TotalNumberOfVerts + this->TotalNumberOfLines +
                  this->TotalNumberOfStrips + this->StartPoly);
  numCells = this->GetNumberOfPolysInPiece(this->Piece);
  memcpy(outArray->GetVoidPointer(outStartCell*components),
         inArray->GetVoidPointer(inStartCell*components), numCells*tupleSize);
}

//----------------------------------------------------------------------------
vtkXMLDataReader* vtkXMLPPolyDataReader::CreatePieceReader()
{
  return vtkXMLPolyDataReader::New();
}

//----------------------------------------------------------------------------
int vtkXMLPPolyDataReader::FillOutputPortInformation(int,
                                                 vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}
