/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPolyDataReader.cxx
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
#include "vtkXMLPolyDataReader.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkPolyData.h"
#include "vtkIdTypeArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCellArray.h"

vtkCxxRevisionMacro(vtkXMLPolyDataReader, "1.1");
vtkStandardNewMacro(vtkXMLPolyDataReader);

//----------------------------------------------------------------------------
vtkXMLPolyDataReader::vtkXMLPolyDataReader()
{
  // Copied from vtkPolyDataReader constructor:
  this->SetOutput(vtkPolyData::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
  
  this->VertElements = 0;
  this->LineElements = 0;
  this->StripElements = 0;
  this->PolyElements = 0;
  this->TotalNumberOfVerts = 0;
  this->TotalNumberOfLines = 0;
  this->TotalNumberOfStrips = 0;
  this->TotalNumberOfPolys = 0;
}

//----------------------------------------------------------------------------
vtkXMLPolyDataReader::~vtkXMLPolyDataReader()
{
  if(this->NumberOfPieces) { this->DestroyPieces(); }
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::SetOutput(vtkPolyData *output)
{
  this->Superclass::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkXMLPolyDataReader::GetOutput()
{
  if(this->NumberOfOutputs < 1)
    {
    return 0;
    }
  return static_cast<vtkPolyData*>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPolyDataReader::GetNumberOfVerts()
{
  return this->TotalNumberOfVerts;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPolyDataReader::GetNumberOfLines()
{
  return this->TotalNumberOfLines;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPolyDataReader::GetNumberOfStrips()
{
  return this->TotalNumberOfStrips;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPolyDataReader::GetNumberOfPolys()
{
  return this->TotalNumberOfPolys;
}

//----------------------------------------------------------------------------
const char* vtkXMLPolyDataReader::GetDataSetName()
{
  return "PolyData";
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::GetOutputUpdateExtent(int& piece,
                                                 int& numberOfPieces,
                                                 int& ghostLevel)
{
  this->GetOutput()->GetUpdateExtent(piece, numberOfPieces, ghostLevel);
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::SetupOutputTotals()
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
    this->TotalNumberOfCells += (this->NumberOfVerts[i] +
                                 this->NumberOfLines[i] +
                                 this->NumberOfStrips[i] +
                                 this->NumberOfPolys[i]);
    this->TotalNumberOfVerts += this->NumberOfVerts[i];
    this->TotalNumberOfLines += this->NumberOfLines[i];
    this->TotalNumberOfStrips += this->NumberOfStrips[i];
    this->TotalNumberOfPolys += this->NumberOfPolys[i];
    }
  
  // Data reading will start at the beginning of the output.
  this->StartVert = 0;
  this->StartLine = 0;
  this->StartStrip = 0;
  this->StartPoly = 0;
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->NumberOfVerts = new vtkIdType[numPieces];
  this->NumberOfLines = new vtkIdType[numPieces];
  this->NumberOfStrips = new vtkIdType[numPieces];
  this->NumberOfPolys = new vtkIdType[numPieces];
  this->VertElements = new vtkXMLDataElement*[numPieces];
  this->LineElements = new vtkXMLDataElement*[numPieces];
  this->StripElements = new vtkXMLDataElement*[numPieces];
  this->PolyElements = new vtkXMLDataElement*[numPieces];
  int i;
  for(i=0;i < numPieces; ++i)
    {
    this->VertElements[i] = 0;
    this->LineElements[i] = 0;
    this->StripElements[i] = 0;
    this->PolyElements[i] = 0;
    }
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::DestroyPieces()
{
  delete [] this->PolyElements;
  delete [] this->StripElements;
  delete [] this->LineElements;
  delete [] this->VertElements;
  delete [] this->NumberOfPolys;
  delete [] this->NumberOfStrips;
  delete [] this->NumberOfLines;
  delete [] this->NumberOfVerts;
  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::SetupOutputData()
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
int vtkXMLPolyDataReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if(!this->Superclass::ReadPiece(ePiece)) { return 0; }
  
  if(!ePiece->GetScalarAttribute("NumberOfVerts",
                                 this->NumberOfVerts[this->Piece]))
    {
    this->NumberOfVerts[this->Piece] = 0;
    }
  if(!ePiece->GetScalarAttribute("NumberOfLines",
                                 this->NumberOfLines[this->Piece]))
    {
    this->NumberOfLines[this->Piece] = 0;
    }
  if(!ePiece->GetScalarAttribute("NumberOfStrips",
                                 this->NumberOfStrips[this->Piece]))
    {
    this->NumberOfStrips[this->Piece] = 0;
    }
  if(!ePiece->GetScalarAttribute("NumberOfPolys",
                                 this->NumberOfPolys[this->Piece]))
    {
    this->NumberOfPolys[this->Piece] = 0;
    }
  
  // Find the cell elements in the piece.
  int i;
  for(i=0; i < ePiece->GetNumberOfNestedElements(); ++i)
    {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if((strcmp(eNested->GetName(), "Verts") == 0)
       && (eNested->GetNumberOfNestedElements() > 1))
      {
      this->VertElements[this->Piece] = eNested;
      }
    if((strcmp(eNested->GetName(), "Lines") == 0)
       && (eNested->GetNumberOfNestedElements() > 1))
      {
      this->LineElements[this->Piece] = eNested;
      }
    if((strcmp(eNested->GetName(), "Strips") == 0)
       && (eNested->GetNumberOfNestedElements() > 1))
      {
      this->StripElements[this->Piece] = eNested;
      }
    if((strcmp(eNested->GetName(), "Polys") == 0)
       && (eNested->GetNumberOfNestedElements() > 1))
      {
      this->PolyElements[this->Piece] = eNested;
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::SetupNextPiece()
{
  this->Superclass::SetupNextPiece();  
  this->StartVert += this->NumberOfVerts[this->Piece];
  this->StartLine += this->NumberOfLines[this->Piece]; 
  this->StartStrip += this->NumberOfStrips[this->Piece];
  this->StartPoly += this->NumberOfPolys[this->Piece];
}

//----------------------------------------------------------------------------
int vtkXMLPolyDataReader::ReadPieceData()
{
  if(!this->Superclass::ReadPieceData()) { return 0; }
  
  vtkPolyData* output = this->GetOutput();
  
  // Read the Verts.
  if(!this->ReadCellArray(this->NumberOfVerts[this->Piece],
                          this->TotalNumberOfVerts,
                          this->VertElements[this->Piece],
                          output->GetVerts()))
    {
    return 0;
    }
  
  // Read the Lines.
  if(!this->ReadCellArray(this->NumberOfLines[this->Piece],
                          this->TotalNumberOfLines,
                          this->LineElements[this->Piece],
                          output->GetLines()))
    {
    return 0;
    }
  
  // Read the Strips.
  if(!this->ReadCellArray(this->NumberOfStrips[this->Piece],
                          this->TotalNumberOfStrips,
                          this->StripElements[this->Piece],
                          output->GetStrips()))
    {
    return 0;
    }
  
  // Read the Polys.
  if(!this->ReadCellArray(this->NumberOfPolys[this->Piece],
                          this->TotalNumberOfPolys,
                          this->PolyElements[this->Piece],
                          output->GetPolys()))
    {
    return 0;
    }  
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPolyDataReader::ReadArrayForCells(vtkXMLDataElement* da,
                                            vtkDataArray* outArray)
{
  vtkIdType components = outArray->GetNumberOfComponents();
  
  // Read the cell data for the Verts in the piece.
  vtkIdType inStartCell = 0;
  vtkIdType outStartCell = this->StartVert;
  vtkIdType numCells = this->NumberOfVerts[this->Piece];  
  if(!this->ReadData(da, outArray->GetVoidPointer(outStartCell*components),
                     outArray->GetDataType(), inStartCell*components,
                     numCells*components)) { return 0; }
  
  // Read the cell data for the Lines in the piece.
  inStartCell += numCells;
  outStartCell = this->TotalNumberOfVerts + this->StartLine;
  numCells = this->NumberOfLines[this->Piece];  
  if(!this->ReadData(da, outArray->GetVoidPointer(outStartCell*components),
                     outArray->GetDataType(), inStartCell*components,
                     numCells*components)) { return 0; }
  
  // Read the cell data for the Strips in the piece.
  inStartCell += numCells;
  outStartCell = (this->TotalNumberOfVerts + this->TotalNumberOfLines +
                  this->StartStrip);
  numCells = this->NumberOfStrips[this->Piece];  
  if(!this->ReadData(da, outArray->GetVoidPointer(outStartCell*components),
                     outArray->GetDataType(), inStartCell*components,
                     numCells*components)) { return 0; }
  
  // Read the cell data for the Polys in the piece.
  inStartCell += numCells;
  outStartCell = (this->TotalNumberOfVerts + this->TotalNumberOfLines +
                  this->TotalNumberOfStrips + this->StartPoly);
  numCells = this->NumberOfPolys[this->Piece];  
  if(!this->ReadData(da, outArray->GetVoidPointer(outStartCell*components),
                     outArray->GetDataType(), inStartCell*components,
                     numCells*components)) { return 0; }
  
  return 1;
}
