/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtentTranslator.cxx
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

#include "vtkExtentTranslator.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkExtentTranslator* vtkExtentTranslator::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkExtentTranslator");
  if(ret)
    {
    return (vtkExtentTranslator*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkExtentTranslator;
}


//----------------------------------------------------------------------------
vtkExtentTranslator::vtkExtentTranslator()
{
  this->MinimumPieceSize[0] = 1;
  this->MinimumPieceSize[1] = 1;
  this->MinimumPieceSize[2] = 1; 
  
  this->Piece = 0;
  this->NumberOfPieces = 0;
  
  this->GhostLevel = 0;
  
  this->Extent[0] = this->Extent[2] = this->Extent[4] = 0; 
  this->Extent[1] = this->Extent[3] = this->Extent[5] = -1; 
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0; 
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1; 
}

//----------------------------------------------------------------------------
vtkExtentTranslator::~vtkExtentTranslator()
{
}

//----------------------------------------------------------------------------
int vtkExtentTranslator::PieceToExtent()
{
  this->GetWholeExtent(this->Extent);
  if (this->SplitExtent(this->Piece, this->NumberOfPieces, this->Extent) == 0)
    {
    // Nothing in this piece.
    this->Extent[0] = this->Extent[2] = this->Extent[4] = 0;
    this->Extent[1] = this->Extent[3] = this->Extent[5] = -1;
    return 0;
    }
  if (this->GhostLevel > 0)
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


//----------------------------------------------------------------------------
int vtkExtentTranslator::SplitExtent(int piece, int numPieces, int *ext)
{
  int numPiecesInFirstHalf;
  int size[3], mid, splitAxis;

  // keep splitting until we have only one piece.
  // piece and numPieces will always be relative to the current ext. 
  while (numPieces > 1)
    {
    // Get the dimensions for each axis.
    size[0] = ext[1]-ext[0];
    size[1] = ext[3]-ext[2];
    size[2] = ext[5]-ext[4];
    // choose the biggest axis
    if (size[2] >= size[1] && size[2] >= size[0] && 
	size[2]/2 >= this->MinimumPieceSize[2])
      {
      splitAxis = 2;
      }
    else if (size[1] >= size[0] && size[1]/2 >= this->MinimumPieceSize[1])
      {
      splitAxis = 1;
      }
    else if (size[0]/2 >= this->MinimumPieceSize[0])
      {
      splitAxis = 0;
      }
    else
      {
      // signal no more splits possible
      splitAxis = -1;
      }

    if (splitAxis == -1)
      {
      // can not split any more.
      if (piece == 0)
        {
        // just return the remaining piece
        numPieces = 1;
        }
      else
        {
        // the rest must be empty
        return 0;
        }
      }
    else
      {
      // split the chosen axis into two pieces.
      numPiecesInFirstHalf = (numPieces / 2);
      mid = (size[splitAxis] * numPiecesInFirstHalf / numPieces) 
	+ ext[splitAxis*2];
      if (piece < numPiecesInFirstHalf)
        {
        // piece is in the first half
        // set extent to the first half of the previous value.
        ext[splitAxis*2+1] = mid;
        // piece must adjust.
        numPieces = numPiecesInFirstHalf;
        }
      else
        {
        // piece is in the second half.
        // set the extent to be the second half. (two halves share points)
        ext[splitAxis*2] = mid;
        // piece must adjust
        numPieces = numPieces - numPiecesInFirstHalf;
        piece -= numPiecesInFirstHalf;
        }
      }
    } // end of while

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtentTranslator::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Piece: " << this->Piece << endl;
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;

  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  
  os << indent << "Extent: " << this->Extent[0] << ", " 
     << this->Extent[1] << ", " << this->Extent[2] << ", " 
     << this->Extent[3] << ", " << this->Extent[4] << ", " 
     << this->Extent[5] << endl; 

  os << indent << "WholeExtent: " << this->WholeExtent[0] << ", " 
     << this->WholeExtent[1] << ", " << this->WholeExtent[2] << ", " 
     << this->WholeExtent[3] << ", " << this->WholeExtent[4] << ", " 
     << this->WholeExtent[5] << endl; 

  os << indent << "MiniumuPieceSize: " << this->MinimumPieceSize[0] << ", "
     << this->MinimumPieceSize[1] << ", " 
     << this->MinimumPieceSize[2] << ", " << "\n";
}







