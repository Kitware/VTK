/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableExtentTranslator.cxx
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
#include "vtkTableExtentTranslator.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkTableExtentTranslator, "1.1");
vtkStandardNewMacro(vtkTableExtentTranslator);

//----------------------------------------------------------------------------
vtkTableExtentTranslator::vtkTableExtentTranslator()
{
  this->ExtentTable = 0;
  this->MaximumGhostLevel = 0;
}

//----------------------------------------------------------------------------
vtkTableExtentTranslator::~vtkTableExtentTranslator()
{
  if(this->ExtentTable)
    {
    delete [] this->ExtentTable;
    this->ExtentTable = 0;
    }
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
    for(i=1;i < this->NumberOfPieces;++i)
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
}

//----------------------------------------------------------------------------
void vtkTableExtentTranslator::SetNumberOfPieces(int pieces)
{
  // Make sure we are really changing the number of pieces.
  if(this->NumberOfPieces == pieces)
    {
    return;
    }
  
  // Cannot change the number of pieces between two non-zero values.
  // If a pipeline tries to use this extent translator with any number
  // of pieces but that stored in our table, it is an error, and
  // another extent translator should be used.
  if((this->NumberOfPieces != 0) && (pieces != 0))
    {
    vtkErrorMacro("Cannot change the number of pieces from "
                  << this->NumberOfPieces << " to " << pieces);
    return;
    }  
  
  // Actually set the NumberOfPieces data member.
  this->Superclass::SetNumberOfPieces(pieces);
  
  // Clean out any old extent table.
  if(this->ExtentTable)
    {
    delete [] this->ExtentTable;
    this->ExtentTable = 0;
    }
  
  // Create and initialize a new extent table if there are any pieces.
  if(this->NumberOfPieces > 0)
    {
    this->ExtentTable = new int[this->NumberOfPieces*6];
    int i;
    for(i=0;i < this->NumberOfPieces;++i)
      {
      int* extent = this->ExtentTable + i*6;
      extent[0] = extent[2] = extent[4] = 0;
      extent[1] = extent[3] = extent[5] = -1;
      }
    }
}

//----------------------------------------------------------------------------
void vtkTableExtentTranslator::SetExtentForPiece(int piece, int* extent)
{
  if((!this->ExtentTable) || (piece < 0) || (piece >= this->NumberOfPieces))
    {
    vtkErrorMacro("Piece " << piece << " does not exist.");
    return;
    }
  memcpy(this->ExtentTable+piece*6, extent, sizeof(int)*6);
}

//----------------------------------------------------------------------------
void vtkTableExtentTranslator::GetExtentForPiece(int piece, int* extent)
{
  if((!this->ExtentTable) || (piece < 0) || (piece >= this->NumberOfPieces))
    {
    vtkErrorMacro("Piece " << piece << " does not exist.");
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
  if((!this->ExtentTable) || (piece < 0) || (piece >= this->NumberOfPieces))
    {
    vtkErrorMacro("Piece " << piece << " does not exist.");
    return emptyExtent;
    }
  return this->ExtentTable+piece*6;
}

//----------------------------------------------------------------------------
int vtkTableExtentTranslator::PieceToExtentByPoints()
{
  vtkErrorMacro("PieceToExtentByPoints not supported.");
  return 0;
}

//----------------------------------------------------------------------------
int
vtkTableExtentTranslator
::PieceToExtentThreadSafe(int vtkNotUsed(piece),
                          int vtkNotUsed(numPieces),
                          int vtkNotUsed(ghostLevel),
                          int* vtkNotUsed(wholeExtent),
                          int* vtkNotUsed(resultExtent),
                          int vtkNotUsed(splitMode),
                          int vtkNotUsed(byPoints))
{
  vtkErrorMacro("PieceToExtentThreadSafe not supported.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkTableExtentTranslator::PieceToExtent()
{
  if((!this->ExtentTable) || (this->Piece < 0) ||
     (this->Piece >= this->NumberOfPieces))
    {
    vtkErrorMacro("Piece " << this->Piece << " does not exist.");
    return 0;
    }
  
  if(this->GhostLevel > this->MaximumGhostLevel)
    {
    vtkWarningMacro("Ghost level " << this->GhostLevel
                    << " is larger than MaximumGhostLevel "
                    << this->MaximumGhostLevel << ".  Using the maximum.");
    this->GhostLevel = this->MaximumGhostLevel;
    }
  
  memcpy(this->Extent, this->ExtentTable+this->Piece*6, sizeof(int)*6);
  
  if(((this->Extent[1] - this->Extent[0] + 1)*
      (this->Extent[3] - this->Extent[2] + 1)*
      (this->Extent[5] - this->Extent[4] + 1)) == 0)
    {
    return 0;
    }

  if(this->GhostLevel > 0)
    {
    this->Extent[0] -= this->GhostLevel;
    this->Extent[1] += this->GhostLevel;
    this->Extent[2] -= this->GhostLevel;
    this->Extent[3] += this->GhostLevel;
    this->Extent[4] -= this->GhostLevel;
    this->Extent[5] += this->GhostLevel;
    
    if (this->Extent[0] < this->WholeExtent[0])
      {
      this->Extent[0] = this->WholeExtent[0];
      }
    if (this->Extent[1] > this->WholeExtent[1])
      {
      this->Extent[1] = this->WholeExtent[1];
      }
    if (this->Extent[2] < this->WholeExtent[2])
      {
      this->Extent[2] = this->WholeExtent[2];
      }
    if (this->Extent[3] > this->WholeExtent[3])
      {
      this->Extent[3] = this->WholeExtent[3];
      }
    if (this->Extent[4] < this->WholeExtent[4])
      {
      this->Extent[4] = this->WholeExtent[4];
      }
    if (this->Extent[5] > this->WholeExtent[5])
      {
      this->Extent[5] = this->WholeExtent[5];
      }
    }
  
  return 1;
}
