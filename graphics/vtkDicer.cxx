/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDicer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkDicer.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkDicer* vtkDicer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkDicer");
  if(ret)
    {
    return (vtkDicer*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDicer;
}




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
    this->NumberOfPieces = ceil((double)numPts/this->NumberOfPointsPerPiece);
    this->MemoryLimit = ceil((double)memSize/this->NumberOfPieces);
    }

  else if ( this->DiceMode == VTK_DICE_MODE_SPECIFIED_NUMBER )
    {
    this->NumberOfPointsPerPiece = ceil((double)numPts/this->NumberOfPieces);
    this->MemoryLimit = ceil((double)memSize/this->NumberOfPieces);
    }

  else //this->DiceMode == VTK_DICE_MODE_MEMORY_LIMIT
    {
    this->NumberOfPieces = ceil((double)memSize/this->MemoryLimit);
    this->NumberOfPointsPerPiece = ceil((double)numPts/this->NumberOfPieces);
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
