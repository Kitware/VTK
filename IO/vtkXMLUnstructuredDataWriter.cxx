/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredDataWriter.cxx
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
#include "vtkXMLUnstructuredDataWriter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataCompressor.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkXMLUnstructuredDataWriter, "1.4");

//----------------------------------------------------------------------------
vtkXMLUnstructuredDataWriter::vtkXMLUnstructuredDataWriter()
{
  this->NumberOfPieces = 1;
  this->WritePiece = -1;
  this->GhostLevel = 0;
  this->CellPoints = vtkIdTypeArray::New();
  this->CellOffsets = vtkIdTypeArray::New();
  this->CellPoints->SetName("connectivity");
  this->CellOffsets->SetName("offsets");
}

//----------------------------------------------------------------------------
vtkXMLUnstructuredDataWriter::~vtkXMLUnstructuredDataWriter()
{
  this->CellPoints->Delete();
  this->CellOffsets->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << "\n";
  os << indent << "WritePiece: " << this->WritePiece << "\n";
  os << indent << "GhostLevel: " << this->GhostLevel << "\n";
}

//----------------------------------------------------------------------------
vtkPointSet* vtkXMLUnstructuredDataWriter::GetInputAsPointSet()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkPointSet*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredDataWriter::WriteData()
{
  vtkIndent indent = vtkIndent().GetNextIndent();
  
  vtkPointSet* input = this->GetInputAsPointSet();
  input->UpdateInformation();
  
  // We don't want to write more pieces than the pipeline can produce,
  // but we need to preserve the user's requested number of pieces in
  // case the input changes later.  If MaximumNumberOfPieces is lower
  // than 1, any number of pieces can be produced by the pipeline.
  int maxPieces = input->GetMaximumNumberOfPieces();
  int numPieces = this->NumberOfPieces;
  if((maxPieces > 0) && (this->NumberOfPieces > maxPieces))
    {
    this->NumberOfPieces = maxPieces;
    }
  
  // Write the file.
  this->StartFile();
  if(this->DataMode == vtkXMLWriter::Appended)
    {
    this->WriteAppendedMode(indent);
    }
  else
    {
    this->WriteInlineMode(indent);
    }
  this->EndFile();
  
  // Restore the user's number of pieces.
  this->NumberOfPieces = numPieces;
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteInlineMode(vtkIndent indent)
{
  ostream& os = *(this->Stream);
  vtkIndent nextIndent = indent.GetNextIndent();
  vtkPointSet* input = this->GetInputAsPointSet();
  
  // Open the primary element.
  os << indent << "<" << this->GetDataSetName() << ">\n";
  
  if((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
    // Loop over each piece and write it.  Unfortunately, there is no
    // way to know the relative size of each piece ahead of time
    // without executing twice for all pieces.  We just assume all the
    // pieces are approximately the same size for reporting progress.
    float progressRange[2] = {0,0};
    this->GetProgressRange(progressRange);
    int i;
    for(i=0; i < this->NumberOfPieces; ++i)
      {
      this->SetProgressRange(progressRange, i, this->NumberOfPieces);
      this->SetInputUpdateExtent(i, this->NumberOfPieces, this->GhostLevel);
      input->Update();
      
      // Open the piece's element.
      os << nextIndent << "<Piece";
      this->WriteInlinePieceAttributes();
      os << ">\n";
      
      this->WriteInlinePiece(nextIndent.GetNextIndent());
      
      // Close the piece's element.
      os << nextIndent << "</Piece>\n";
      }
    }
  else
    {
    // Write just the one requested piece.
    this->SetInputUpdateExtent(this->WritePiece, this->NumberOfPieces,
                               this->GhostLevel);
    input->Update();
    
    // Open the piece's element.
    os << nextIndent << "<Piece";
    this->WriteInlinePieceAttributes();
    os << ">\n";
    
    this->WriteInlinePiece(nextIndent.GetNextIndent());
    
    // Close the piece's element.
    os << nextIndent << "</Piece>\n";
    }
  
  // Close the primary element.
  os << indent << "</" << this->GetDataSetName() << ">\n";
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteInlinePieceAttributes()
{
  vtkPointSet* input = this->GetInputAsPointSet();
  this->WriteScalarAttribute("NumberOfPoints",
                             input->GetNumberOfPoints());
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteInlinePiece(vtkIndent indent)
{
  vtkPointSet* input = this->GetInputAsPointSet();
  
  // Split progress among point data, cell data, and point arrays.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[4];
  this->CalculateDataFractions(fractions);
  
  // Set the range of progress for the point data arrays.
  this->SetProgressRange(progressRange, 0, fractions);  
  
  // Write the point data arrays.
  this->WritePointDataInline(input->GetPointData(), indent);
  
  // Set the range of progress for the cell data arrays.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the cell data arrays.
  this->WriteCellDataInline(input->GetCellData(), indent);
  
  // Set the range of progress for the point specification array.
  this->SetProgressRange(progressRange, 2, fractions);
  
  // Write the point specification array.
  this->WritePointsInline(input->GetPoints(), indent);
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteAppendedMode(vtkIndent indent)
{
  ostream& os = *(this->Stream);
  vtkIndent nextIndent = indent.GetNextIndent();
  
  this->NumberOfPointsPositions = new unsigned long[this->NumberOfPieces];
  this->PointsPositions = new unsigned long[this->NumberOfPieces];
  this->PointDataPositions = new unsigned long*[this->NumberOfPieces];
  this->CellDataPositions = new unsigned long*[this->NumberOfPieces];
  
  // Update the first piece of the input to get the form of the data.
  vtkPointSet* input = this->GetInputAsPointSet();
  int piece = this->WritePiece;
  if((piece < 0) || (piece >= this->NumberOfPieces)) { piece = 0; }
  input->SetUpdateExtent(piece, this->NumberOfPieces, this->GhostLevel);
  input->Update();
  
  // Open the primary element.
  os << indent << "<" << this->GetDataSetName() << ">\n";
  
  if((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
    // Loop over each piece and write its structure.
    int i;
    for(i=0; i < this->NumberOfPieces; ++i)
      {
      // Open the piece's element.
      os << nextIndent << "<Piece";
      this->WriteAppendedPieceAttributes(i);
      os << ">\n";
      
      this->WriteAppendedPiece(i, nextIndent.GetNextIndent());
      
      // Close the piece's element.
      os << nextIndent << "</Piece>\n";
      }
    }
  else
    {
    // Write just the requested piece.
    // Open the piece's element.
    os << nextIndent << "<Piece";
    this->WriteAppendedPieceAttributes(this->WritePiece);
    os << ">\n";
    
    this->WriteAppendedPiece(this->WritePiece, nextIndent.GetNextIndent());
    
    // Close the piece's element.
    os << nextIndent << "</Piece>\n";
    }
  
  // Close the primary element.
  os << indent << "</" << this->GetDataSetName() << ">\n";
  
  this->StartAppendedData();
  
  if((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
    // Loop over each piece and write its data.  Unfortunately, there
    // is no way to know the relative size of each piece ahead of time
    // without executing twice for all pieces.  We just assume all the
    // pieces are approximately the same size for reporting progress.
    float progressRange[2] = {0,0};
    this->GetProgressRange(progressRange);
    int i;
    for(i=0; i < this->NumberOfPieces; ++i)
      {
      this->SetProgressRange(progressRange, i, this->NumberOfPieces);
      input->SetUpdateExtent(i, this->NumberOfPieces, this->GhostLevel);
      input->Update();
      this->WriteAppendedPieceData(i);
      }
    }
  else
    {
    // Write just the requested piece.
    input->SetUpdateExtent(this->WritePiece, this->NumberOfPieces,
                           this->GhostLevel);
    input->Update();
    this->WriteAppendedPieceData(this->WritePiece);
    }
  
  this->EndAppendedData();
  
  delete [] this->NumberOfPointsPositions;
  delete [] this->PointsPositions;
  delete [] this->PointDataPositions;
  delete [] this->CellDataPositions;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteAppendedPieceAttributes(int index)
{
  this->NumberOfPointsPositions[index] =
    this->ReserveAttributeSpace("NumberOfPoints");
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteAppendedPiece(int index,
                                                      vtkIndent indent)
{
  vtkPointSet* input = this->GetInputAsPointSet();
  
  this->PointDataPositions[index] =
    this->WritePointDataAppended(input->GetPointData(), indent);
  
  this->CellDataPositions[index] =
    this->WriteCellDataAppended(input->GetCellData(), indent);
  
  this->PointsPositions[index] =
    this->WritePointsAppended(input->GetPoints(), indent);
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteAppendedPieceData(int index)
{
  ostream& os = *(this->Stream);
  vtkPointSet* input = this->GetInputAsPointSet();
  
  unsigned long returnPosition = os.tellp();
  os.seekp(this->NumberOfPointsPositions[index]);
  vtkPoints* points = input->GetPoints();
  this->WriteScalarAttribute("NumberOfPoints",
                             (points?points->GetNumberOfPoints():0));
  os.seekp(returnPosition);
  
  // Split progress among point data, cell data, and point arrays.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[4];
  this->CalculateDataFractions(fractions);
  
  // Set the range of progress for the point data arrays.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Write the point data arrays.
  this->WritePointDataAppendedData(input->GetPointData(),
                                   this->PointDataPositions[index]);
  
  // Set the range of progress for the cell data arrays.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the cell data arrays.  
  this->WriteCellDataAppendedData(input->GetCellData(),
                                  this->CellDataPositions[index]);
  
  // Set the range of progress for the point specification array.
  this->SetProgressRange(progressRange, 2, fractions);
  
  // Write the point specification array.
  this->WritePointsAppendedData(input->GetPoints(),
                                this->PointsPositions[index]);
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsInline(const char* name,
                                                    vtkCellArray* cells,
                                                    vtkDataArray* types,
                                                    vtkIndent indent)
{
  this->ConvertCells(cells);
  
  ostream& os = *(this->Stream);
  os << indent << "<" << name << ">\n";
  
  // Split progress by cell connectivity, offset, and type arrays.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[4];
  this->CalculateCellFractions(fractions, types?types->GetNumberOfTuples():0);
  
  // Set the range of progress for the connectivity array.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Write the connectivity array.
  this->WriteDataArrayInline(this->CellPoints, indent.GetNextIndent());
  
  // Set the range of progress for the offsets array.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the offsets array.
  this->WriteDataArrayInline(this->CellOffsets, indent.GetNextIndent());
  if(types)
    {
    // Set the range of progress for the types array.
    this->SetProgressRange(progressRange, 2, fractions);
    
    // Write the types array.
    this->WriteDataArrayInline(types, indent.GetNextIndent(), "types");
    }
  os << indent << "</" << name << ">\n";
}

//----------------------------------------------------------------------------
unsigned long*
vtkXMLUnstructuredDataWriter::WriteCellsAppended(const char* name,
                                                 vtkDataArray* types,
                                                 vtkIndent indent)
{
  unsigned long* positions = new unsigned long[3];
  ostream& os = *(this->Stream);
  os << indent << "<" << name << ">\n";
  positions[0] = this->WriteDataArrayAppended(this->CellPoints,
                                              indent.GetNextIndent());
  positions[1] = this->WriteDataArrayAppended(this->CellOffsets,
                                              indent.GetNextIndent());
  if(types)
    {
    positions[2] = this->WriteDataArrayAppended(types, indent.GetNextIndent(),
                                                "types");
    }
  os << indent << "</" << name << ">\n";
  return positions;
}

//----------------------------------------------------------------------------
void
vtkXMLUnstructuredDataWriter::WriteCellsAppendedData(vtkCellArray* cells,
                                                     vtkDataArray* types,
                                                     unsigned long* positions)
{
  this->ConvertCells(cells);
  
  // Split progress by cell connectivity, offset, and type arrays.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[4];
  this->CalculateCellFractions(fractions, types?types->GetNumberOfTuples():0);
  
  // Set the range of progress for the connectivity array.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Write the connectivity array.
  this->WriteDataArrayAppendedData(this->CellPoints, positions[0]);
  
  // Set the range of progress for the offsets array.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the offsets array.
  this->WriteDataArrayAppendedData(this->CellOffsets, positions[1]);
  
  if(types)
    {
    // Set the range of progress for the types array.
    this->SetProgressRange(progressRange, 2, fractions);
    
    // Write the types array.
    this->WriteDataArrayAppendedData(types, positions[2]);
    }
  delete [] positions;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::ConvertCells(vtkCellArray* cells)
{
  vtkIdTypeArray* connectivity = cells->GetData();
  vtkIdType numberOfCells = cells->GetNumberOfCells();
  vtkIdType numberOfTuples = connectivity->GetNumberOfTuples();
  
  this->CellPoints->SetNumberOfTuples(numberOfTuples - numberOfCells);
  this->CellOffsets->SetNumberOfTuples(numberOfCells);
  
  vtkIdType* inCell = connectivity->GetPointer(0);
  vtkIdType* outCellPointsBase = this->CellPoints->GetPointer(0);
  vtkIdType* outCellPoints = outCellPointsBase;
  vtkIdType* outCellOffset = this->CellOffsets->GetPointer(0);
  
  vtkIdType i;
  for(i=0;i < numberOfCells; ++i)
    {
    vtkIdType numberOfPoints = *inCell++;
    memcpy(outCellPoints, inCell, sizeof(vtkIdType)*numberOfPoints);
    outCellPoints += numberOfPoints;
    inCell += numberOfPoints;
    *outCellOffset++ = outCellPoints - outCellPointsBase;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredDataWriter::GetNumberOfInputPoints()
{
  vtkPointSet* input = this->GetInputAsPointSet();
  vtkPoints* points = input->GetPoints();
  return points?points->GetNumberOfPoints():0;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::CalculateDataFractions(float* fractions)
{
  // Calculate the fraction of point/cell data and point
  // specifications contributed by each component.
  vtkPointSet* input = this->GetInputAsPointSet();
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  vtkIdType pdSize = pdArrays*this->GetNumberOfInputPoints();
  vtkIdType cdSize = cdArrays*this->GetNumberOfInputCells();
  int total = (pdSize+cdSize+this->GetNumberOfInputPoints());
  if(total == 0)
    {
    total = 1;
    }
  fractions[0] = 0;
  fractions[1] = float(pdSize)/total;
  fractions[2] = float(pdSize+cdSize)/total;
  fractions[3] = 1;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::CalculateCellFractions(float* fractions,
                                                          vtkIdType typesSize)
{
  // Calculate the fraction of cell specification data contributed by
  // each of the connectivity, offset, and type arrays.
  vtkIdType connectSize = this->CellPoints->GetNumberOfTuples();
  vtkIdType offsetSize = this->CellOffsets->GetNumberOfTuples();
  vtkIdType total = connectSize+offsetSize+typesSize;
  if(total == 0)
    {
    total = 1;
    }
  fractions[0] = 0;
  fractions[1] = float(connectSize)/total;
  fractions[2] = float(connectSize+offsetSize)/total;
  fractions[3] = 1;
}
