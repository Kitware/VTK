/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDicer.cxx
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
#include "vtkDicer.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkDicer, "1.30");

// Instantiate object.
vtkDicer::vtkDicer()
{
  this->NumberOfPointsPerPiece = 5000;
  this->NumberOfPieces = 10;
  this->MemoryLimit = 50000; //50 MBytes
  this->NumberOfActualPieces = 0;
  this->FieldData = 0;
  this->DiceMode = VTK_DICE_MODE_NUMBER_OF_POINTS;
}

// This method unifies the measures used to define piece size. Call this
// in the subclass Execute() method.
void vtkDicer::UpdatePieceMeasures()
{
  vtkDataSet *input = this->GetInput();
  int numPts = input->GetNumberOfPoints();
  unsigned long memSize = input->GetActualMemorySize();

  if ( this->DiceMode == VTK_DICE_MODE_NUMBER_OF_POINTS )
    {
    this->NumberOfPieces = (int) ceil((double)numPts/this->NumberOfPointsPerPiece);
    this->MemoryLimit = (unsigned long) ceil((double)memSize/this->NumberOfPieces);
    }

  else if ( this->DiceMode == VTK_DICE_MODE_SPECIFIED_NUMBER )
    {
    this->NumberOfPointsPerPiece = (int) ceil((double)numPts/this->NumberOfPieces);
    this->MemoryLimit = (unsigned long) ceil((double)memSize/this->NumberOfPieces);
    }

  else //this->DiceMode == VTK_DICE_MODE_MEMORY_LIMIT
    {
    this->NumberOfPieces = (int) ceil((double)memSize/this->MemoryLimit);
    this->NumberOfPointsPerPiece = (int) ceil((double)numPts/this->NumberOfPieces);
    }
}

void vtkDicer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of Points per Piece: " 
     << this->NumberOfPointsPerPiece << "\n";

  os << indent << "Number of Pieces: " 
     << this->NumberOfPieces << "\n";

  os << indent << "Memory Limit: " 
     << this->MemoryLimit << "\n";

  os << indent << "Number of Actual Pieces: " 
     << this->NumberOfActualPieces << "\n";

  os << indent << "Field Data: " << (this->FieldData ? "On\n" : "Off\n");

  if ( this->DiceMode == VTK_DICE_MODE_NUMBER_OF_POINTS )
    {
    os << indent << "Dice Mode: Number Of Points\n";
    }
  else if ( this->DiceMode == VTK_DICE_MODE_SPECIFIED_NUMBER )
    {
    os << indent << "Dice Mode: Specified Number\n";
    }
  else
    {
    os << indent << "Dice Mode: Memory Limit\n";
    }
}
