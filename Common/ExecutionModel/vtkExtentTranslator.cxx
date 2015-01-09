/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtentTranslator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtentTranslator.h"
#include "vtkObjectFactory.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerRequestKey.h"
#include "vtkLargeInteger.h"

vtkStandardNewMacro(vtkExtentTranslator);

vtkInformationKeyMacro(vtkExtentTranslator, DATA_SPLIT_MODE, Integer);

// Subclass vtkInformationIntegerRequestKey to set the DataKey.
class vtkInformationSplitModeRequestKey : public vtkInformationIntegerRequestKey
{
public:
  vtkInformationSplitModeRequestKey(const char* name, const char* location) :
    vtkInformationIntegerRequestKey(name, location)
    {
    this->DataKey = vtkExtentTranslator::DATA_SPLIT_MODE();
    }
};
vtkInformationKeySubclassMacro(vtkExtentTranslator, UPDATE_SPLIT_MODE,
                               SplitModeRequest, IntegerRequest);

//----------------------------------------------------------------------------
vtkExtentTranslator::vtkExtentTranslator()
{
  this->Piece = 0;
  this->NumberOfPieces = 0;

  this->GhostLevel = 0;

  this->Extent[0] = this->Extent[2] = this->Extent[4] = 0;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = -1;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;

  // Set a default split mode to be slabs
  this->SplitMode   = vtkExtentTranslator::BLOCK_MODE;

  this->SplitLen = 0;
  this->SplitPath = NULL;
}

//----------------------------------------------------------------------------
vtkExtentTranslator::~vtkExtentTranslator()
{
  this->SetSplitPath(0, NULL);
}

//----------------------------------------------------------------------------
void vtkExtentTranslator::SetSplitPath(int len, int *sp)
{
  delete[] this->SplitPath;
  this->SplitPath = NULL;
  this->SplitLen = len;
  if (len && sp)
    {
    this->SplitPath = new int[len];
    memcpy(this->SplitPath, sp, len*sizeof(int));
    }
}

//----------------------------------------------------------------------------
int vtkExtentTranslator::PieceToExtent()
{
  return
    this->PieceToExtentThreadSafe(this->Piece, this->NumberOfPieces,
                                  this->GhostLevel, this->WholeExtent,
                                  this->Extent, this->SplitMode, 0);
}

//----------------------------------------------------------------------------
int vtkExtentTranslator::PieceToExtentByPoints()
{
  return
    this->PieceToExtentThreadSafe(this->Piece, this->NumberOfPieces,
                                  this->GhostLevel, this->WholeExtent,
                                  this->Extent, this->SplitMode, 1);
}

int vtkExtentTranslator::PieceToExtentThreadSafe(int piece, int numPieces,
                                                 int ghostLevel,
                                                 int *wholeExtent,
                                                 int *resultExtent,
                                                 int splitMode,
                                                 int byPoints)
{
  memcpy(resultExtent, wholeExtent, sizeof(int)*6);
  int ret;
  if (byPoints)
    {
    ret = this->SplitExtentByPoints(piece, numPieces, resultExtent, splitMode);
    }
  else
    {
    ret = this->SplitExtent(piece, numPieces, resultExtent, splitMode);
    }

  if (ret == 0)
    {
    // Nothing in this piece.
    resultExtent[0] = resultExtent[2] = resultExtent[4] = 0;
    resultExtent[1] = resultExtent[3] = resultExtent[5] = -1;
    return 0;
    }
  if (ghostLevel > 0)
    {
    resultExtent[0] -= ghostLevel;
    resultExtent[1] += ghostLevel;
    resultExtent[2] -= ghostLevel;
    resultExtent[3] += ghostLevel;
    resultExtent[4] -= ghostLevel;
    resultExtent[5] += ghostLevel;

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
int vtkExtentTranslator::SplitExtent(int piece, int numPieces, int *ext,
                                     int splitMode)
{
  int numPiecesInFirstHalf;
  unsigned long size[3];
  int splitAxis;
  vtkLargeInteger mid;

  if (piece >= numPieces || piece < 0)
    {
    return 0;
    }

  // keep splitting until we have only one piece.
  // piece and numPieces will always be relative to the current ext.
  int cnt = 0;
  while (numPieces > 1)
    {
    // Get the dimensions for each axis.
    size[0] = ext[1]-ext[0];
    size[1] = ext[3]-ext[2];
    size[2] = ext[5]-ext[4];
    // choose what axis to split on based on the SplitMode
    // if the user has requested x, y, or z slabs then try to
    // honor that request. If that axis is already split as
    // far as it can go, then drop to block mode.
    if (this->SplitPath && cnt<this->SplitLen)
      {
      splitMode = this->SplitPath[cnt];
      cnt++;
      }
    if (splitMode < 3 && size[splitMode] > 1)
      {
      splitAxis = splitMode;
      }
    // otherwise use block mode
    else
      {
      // choose the biggest axis
      if (size[2] >= size[1] && size[2] >= size[0] && size[2]/2 >= 1)
        {
        splitAxis = 2;
        }
      else if (size[1] >= size[0] && size[1]/2 >= 1)
        {
        splitAxis = 1;
        }
      else if (size[0]/2 >= 1)
        {
        splitAxis = 0;
        }
      else
        {
        // signal no more splits possible
        splitAxis = -1;
        }
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
      mid = size[splitAxis];
      mid = (mid *  numPiecesInFirstHalf) / numPieces + ext[splitAxis*2];
      if (piece < numPiecesInFirstHalf)
        {
        // piece is in the first half
        // set extent to the first half of the previous value.
        ext[splitAxis*2+1] = mid.CastToInt();
        // piece must adjust.
        numPieces = numPiecesInFirstHalf;
        }
      else
        {
        // piece is in the second half.
        // set the extent to be the second half. (two halves share points)
        ext[splitAxis*2] = mid.CastToInt();
        // piece must adjust
        numPieces = numPieces - numPiecesInFirstHalf;
        piece -= numPiecesInFirstHalf;
        }
      }
    } // end of while

  return 1;
}



//----------------------------------------------------------------------------
int vtkExtentTranslator::SplitExtentByPoints(int piece, int numPieces,
                                             int *ext, int splitMode)
{
  int numPiecesInFirstHalf;
  int size[3], splitAxis;
  vtkLargeInteger mid;

  // keep splitting until we have only one piece.
  // piece and numPieces will always be relative to the current ext.
  while (numPieces > 1)
    {
    // Get the dimensions for each axis.
    size[0] = ext[1]-ext[0] + 1;
    size[1] = ext[3]-ext[2] + 1;
    size[2] = ext[5]-ext[4] + 1;
    // choose what axis to split on based on the SplitMode
    // if the user has requested x, y, or z slabs then try to
    // honor that request. If that axis is already split as
    // far as it can go, then drop to block mode.
    if (splitMode < 3 && size[splitMode] > 1)
      {
      splitAxis = splitMode;
      }
    // otherwise use block mode
    else
      {
      if (size[2] >= size[1] && size[2] >= size[0] && size[2]/2 >= 1)
        {
        splitAxis = 2;
        }
      else if (size[1] >= size[0] && size[1]/2 >= 1)
        {
        splitAxis = 1;
        }
      else if (size[0]/2 >= 1)
        {
        splitAxis = 0;
        }
      else
        {
        // signal no more splits possible
        splitAxis = -1;
        }
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
      mid = size[splitAxis];
      mid = (mid *  numPiecesInFirstHalf) / numPieces + ext[splitAxis*2];
      if (piece < numPiecesInFirstHalf)
        {
        // piece is in the first half
        // set extent to the first half of the previous value.
        ext[splitAxis*2+1] = mid.CastToInt() - 1;
        // piece must adjust.
        numPieces = numPiecesInFirstHalf;
        }
      else
        {
        // piece is in the second half.
        // set the extent to be the second half.
        ext[splitAxis*2] = mid.CastToInt();
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
  this->Superclass::PrintSelf(os,indent);

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

  os << indent << "SplitMode: ";
  if (this->SplitMode == vtkExtentTranslator::BLOCK_MODE)
    {
    os << "Block\n";
    }
  else if (this->SplitMode == vtkExtentTranslator::X_SLAB_MODE)
    {
    os << "X Slab\n";
    }
  else if (this->SplitMode == vtkExtentTranslator::Y_SLAB_MODE)
    {
    os << "Y Slab\n";
    }
  else if (this->SplitMode == vtkExtentTranslator::Z_SLAB_MODE)
    {
    os << "Z Slab\n";
    }
  else
    {
    os << "Unknown\n";
    }
}
