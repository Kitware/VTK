/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPPolyDataReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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

vtkCxxRevisionMacro(vtkXMLPPolyDataReader, "1.1");
vtkStandardNewMacro(vtkXMLPPolyDataReader);

//----------------------------------------------------------------------------
vtkXMLPPolyDataReader::vtkXMLPPolyDataReader()
{
  // Copied from vtkPolyDataReader constructor:
  this->SetOutput(vtkPolyData::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();  
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
void vtkXMLPPolyDataReader::SetOutput(vtkPolyData *output)
{
  this->Superclass::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkXMLPPolyDataReader::GetOutput()
{
  if(this->NumberOfOutputs < 1)
    {
    return 0;
    }
  return static_cast<vtkPolyData*>(this->Outputs[0]);
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
  this->GetOutput()->GetUpdateExtent(piece, numberOfPieces, ghostLevel);
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPPolyDataReader::GetNumberOfCellsInPiece(int piece)
{
  if(this->PieceReaders[piece])
    {
    return this->PieceReaders[piece]->GetNumberOfCells();
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPPolyDataReader::GetNumberOfVertsInPiece(int piece)
{
  if(this->PieceReaders[piece])
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
  if(this->PieceReaders[piece])
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
  if(this->PieceReaders[piece])
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
  if(this->PieceReaders[piece])
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
  int i;
  this->TotalNumberOfCells = 0;
  this->TotalNumberOfVerts = 0;
  this->TotalNumberOfLines = 0;
  this->TotalNumberOfStrips = 0;
  this->TotalNumberOfPolys = 0;
  for(i=this->StartPiece; i < this->EndPiece; ++i)
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
  
  vtkPolyData* output = this->GetOutput();
  
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
  if(!this->Superclass::ReadPieceData()) { return 0; }
  
  vtkPointSet* ips = this->GetPieceInputAsPointSet(this->Piece);
  vtkPolyData* input = static_cast<vtkPolyData*>(ips);
  vtkPolyData* output = this->GetOutput();
  
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
  if(!this->PieceReaders[this->Piece])
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
