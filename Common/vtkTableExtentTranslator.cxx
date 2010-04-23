/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableExtentTranslator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTableExtentTranslator.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkTableExtentTranslator);

//----------------------------------------------------------------------------
vtkTableExtentTranslator::vtkTableExtentTranslator()
{
  this->ExtentTable = 0;
  this->MaximumGhostLevel = 0;
  this->PieceAvailable = 0;
  this->NumberOfPiecesInTable = 0;
}

//----------------------------------------------------------------------------
vtkTableExtentTranslator::~vtkTableExtentTranslator()
{
  this->SetNumberOfPiecesInTable(0);
}

//----------------------------------------------------------------------------
void vtkTableExtentTranslator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if(this->ExtentTable)
    {
    vtkIndent nextIndent = indent.GetNextIndent();
    int* extent = this->ExtentTable;
    int i;
    
    os << indent << "ExtentTable: 0: "
       << extent[0] << " " << extent[1] << " "
       << extent[2] << " " << extent[3] << " "
       << extent[4] << " " << extent[5] << "\n";
    for(i=1;i < this->NumberOfPiecesInTable;++i)
      {
      extent += 6;
      os << nextIndent << "             " << i << ": "
         << extent[0] << " " << extent[1] << " "
         << extent[2] << " " << extent[3] << " "
         << extent[4] << " " << extent[5] << "\n";
      }
    }
  else
    {
    os << indent << "ExtentTable: (none)\n";
    }
  os << indent << "MaximumGhostLevel: " << this->MaximumGhostLevel << "\n";
  os << indent << "NumberOfPiecesInTable: " 
     << this->NumberOfPiecesInTable << "\n";
  if(this->PieceAvailable)
    {
    vtkIndent nextIndent = indent.GetNextIndent();
    int* available = this->PieceAvailable;
    int i;
    
    os << indent << "PieceAvailable: 0: " << *available << "\n";
    for(i=1;i < this->NumberOfPiecesInTable;++i)
      {
      ++available;
      os << nextIndent << "                " << i << ": "
         << *available << "\n";
      }
    }
  else
    {
    os << indent << "PieceAvailable: (none)\n";
    }
}

//----------------------------------------------------------------------------
void vtkTableExtentTranslator::SetNumberOfPieces(int pieces)
{
  // Allocate a table for this number of pieces.
  if(this->NumberOfPiecesInTable == 0)
    {
    this->SetNumberOfPiecesInTable(pieces);
    }
  this->Superclass::SetNumberOfPieces(pieces);
}

//----------------------------------------------------------------------------
void vtkTableExtentTranslator::SetNumberOfPiecesInTable(int pieces)
{
  // Make sure we are really changing the number of pieces.
  if(this->NumberOfPiecesInTable == pieces)
    {
    return;
    }

  // The default number of pieces returned is the real number of
  // pieces.
  this->Superclass::SetNumberOfPieces(pieces);
  this->NumberOfPiecesInTable = pieces;

  // Clean out any old extent table.
  if(this->ExtentTable)
    {
    delete [] this->ExtentTable;
    this->ExtentTable = 0;
    }
  if(this->PieceAvailable)
    {
    delete [] this->PieceAvailable;
    this->PieceAvailable = 0;
    }
  
  // Create and initialize a new extent table if there are any pieces.
  // Assume all pieces are available.
  if(this->NumberOfPiecesInTable > 0)
    {
    this->ExtentTable = new int[this->NumberOfPiecesInTable*6];
    this->PieceAvailable = new int[this->NumberOfPiecesInTable];
    int i;
    for(i=0;i < this->NumberOfPiecesInTable;++i)
      {
      int* extent = this->ExtentTable + i*6;
      extent[0] = extent[2] = extent[4] = 0;
      extent[1] = extent[3] = extent[5] = -1;
      this->PieceAvailable[i] = 1;
      }
    }
}

//----------------------------------------------------------------------------
void vtkTableExtentTranslator::SetExtentForPiece(int piece, int* extent)
{
  if((!this->ExtentTable) || (piece < 0) ||
     (piece >= this->NumberOfPiecesInTable))
    {
    vtkErrorMacro("Piece " << piece << " does not exist.  "
                  "NumberOfPiecesInTable is " << this->NumberOfPiecesInTable);
    return;
    }
  memcpy(this->ExtentTable+piece*6, extent, sizeof(int)*6);
}

//----------------------------------------------------------------------------
void vtkTableExtentTranslator::GetExtentForPiece(int piece, int* extent)
{
  if((!this->ExtentTable) || (piece < 0) ||
     (piece >= this->NumberOfPiecesInTable))
    {
    vtkErrorMacro("Piece " << piece << " does not exist.  "
                  "NumberOfPiecesInTable is " << this->NumberOfPiecesInTable);
    extent[0] = extent[2] = extent[4] = 0;
    extent[1] = extent[3] = extent[5] = -1;
    return;
    }
  memcpy(extent, this->ExtentTable+piece*6, sizeof(int)*6);
}

//----------------------------------------------------------------------------
int* vtkTableExtentTranslator::GetExtentForPiece(int piece)
{
  static int emptyExtent[6] = {0,-1,0,-1,0,-1};
  if((!this->ExtentTable) || (piece < 0) ||
     (piece >= this->NumberOfPiecesInTable))
    {
    vtkErrorMacro("Piece " << piece << " does not exist.  "
                  "NumberOfPiecesInTable is " << this->NumberOfPiecesInTable);
    return emptyExtent;
    }
  return this->ExtentTable+piece*6;
}

//----------------------------------------------------------------------------
void vtkTableExtentTranslator::SetPieceAvailable(int piece, int available)
{
  if((!this->ExtentTable) || (piece < 0) ||
     (piece >= this->NumberOfPiecesInTable))
    {
    vtkErrorMacro("Piece " << piece << " does not exist.  "
                  "NumberOfPiecesInTable is " << this->NumberOfPiecesInTable);
    return;
    }
  this->PieceAvailable[piece] = available?1:0;
}

//----------------------------------------------------------------------------
int vtkTableExtentTranslator::GetPieceAvailable(int piece)
{
  if((!this->ExtentTable) || (piece < 0) ||
     (piece >= this->NumberOfPiecesInTable))
    {
    vtkErrorMacro("Piece " << piece << " does not exist.  "
                  "NumberOfPiecesInTable is " << this->NumberOfPiecesInTable);
    return 0;
    }
  return this->PieceAvailable[piece];
}

//----------------------------------------------------------------------------
int vtkTableExtentTranslator::PieceToExtentByPoints()
{
  vtkErrorMacro("PieceToExtentByPoints not supported.");
  return 0;
}

//----------------------------------------------------------------------------
int
vtkTableExtentTranslator::PieceToExtentThreadSafe(int piece, int numPieces, 
                                                  int ghostLevel, 
                                                  int *wholeExtent, 
                                                  int *resultExtent, 
                                                  int vtkNotUsed(splitMode), 
                                                  int byPoints)
{
  if (byPoints)
    {
    vtkErrorMacro("PieceToExtentByPoints not supported.");
    return 0;
    }

  if((!this->ExtentTable) || (piece < 0) ||
     (piece >= numPieces))
    {
    vtkErrorMacro("Piece " << piece << " does not exist.");
    return 0;
    }
  
  if(ghostLevel > this->MaximumGhostLevel)
    {
    vtkWarningMacro("Ghost level " << ghostLevel
                    << " is larger than MaximumGhostLevel "
                    << this->MaximumGhostLevel << ".  Using the maximum.");
    ghostLevel = this->MaximumGhostLevel;
    }

  if(numPieces == 1)
    {
    // The number of pieces requested is one.  Return the whole extent.
    memcpy(resultExtent, wholeExtent, sizeof(int)*6);
    }
  else if(piece < this->NumberOfPiecesInTable)
    {
    // Return the extent from the table entry.
    memcpy(resultExtent, this->ExtentTable+piece*6, sizeof(int)*6);
    }
  else
    {
    // The requested piece is beyond the table.  Return an empty extent.
    resultExtent[0] = 0;
    resultExtent[1] = -1;
    resultExtent[2] = 0;
    resultExtent[3] = -1;
    resultExtent[4] = 0;
    resultExtent[5] = -1;
    }

  if(((resultExtent[1] - resultExtent[0] + 1)*
      (resultExtent[3] - resultExtent[2] + 1)*
      (resultExtent[5] - resultExtent[4] + 1)) == 0)
    {
    return 0;
    }

  if(ghostLevel > 0)
    {
    resultExtent[0] -= this->GhostLevel;
    resultExtent[1] += this->GhostLevel;
    resultExtent[2] -= this->GhostLevel;
    resultExtent[3] += this->GhostLevel;
    resultExtent[4] -= this->GhostLevel;
    resultExtent[5] += this->GhostLevel;
    
    if (resultExtent[0] < wholeExtent[0])
      {
      resultExtent[0] = wholeExtent[0];
      }
    if (resultExtent[1] > wholeExtent[1])
      {
      resultExtent[1] = wholeExtent[1];
      }
    if (resultExtent[2] < wholeExtent[2])
      {
      resultExtent[2] = wholeExtent[2];
      }
    if (resultExtent[3] > wholeExtent[3])
      {
      resultExtent[3] = wholeExtent[3];
      }
    if (resultExtent[4] < wholeExtent[4])
      {
      resultExtent[4] = wholeExtent[4];
      }
    if (resultExtent[5] > wholeExtent[5])
      {
      resultExtent[5] = wholeExtent[5];
      }
    }
  
  return 1;

}

//----------------------------------------------------------------------------
int vtkTableExtentTranslator::PieceToExtent()
{
  return this->PieceToExtentThreadSafe(this->Piece, this->NumberOfPieces,
                                       this->GhostLevel,
                                       this->WholeExtent,
                                       this->Extent,
                                       this->SplitMode,
                                       0);
}
