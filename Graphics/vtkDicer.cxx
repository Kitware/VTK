/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDicer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkDicer.h"
#include "vtkMath.h"

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
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

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
