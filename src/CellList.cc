/*=========================================================================

  Program:   Visualization Library
  Module:    CellList.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "CellList.hh"

vlCellList::vlCellList(const int sz, const int ext)
{
  this->Size = sz;
  this->Array = new vlCell_s[sz];
  this->Extend = ext;
  this->MaxId = -1;
}

vlCellList::~vlCellList()
{
  delete [] this->Array;
}

// Description:
// Add a cell to structure
void vlCellList::InsertCell(const int cellId, const unsigned char type, const int loc)
{
  vlCell_s *cell;

  if ( cellId >= this->Size ) this->Resize(cellId);
  if ( cellId > this->MaxId ) this->MaxId = cellId;

  cell = this->Array + cellId;
  cell->type = type;
  cell->loc = loc;

  return;
}

// Description:
// Add a cell to the object in the next available slot.
int vlCellList::InsertNextCell(const unsigned char type, const int loc)
{
  this->InsertCell (++this->MaxId,type,loc);
  return this->MaxId;
}

// Description:
// Reclaim any extra memory.
void vlCellList::Squeeze()
{
  this->Resize (this->MaxId+1);
}

// Description:
// Initialize object without releasing memory.
void vlCellList::Reset()
{
  this->MaxId = -1;
}
//
// Private function does "reallocate"
//
vlCell_s *vlCellList::Resize(const int sz)
{
  int i;
  vlCell_s *newArray;
  int newSize;

  if ( sz >= this->Size )  newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  newArray = new vlCell_s[newSize];

  for (i=0; i<sz && i<this->Size; i++)
      newArray[i] = this->Array[i];

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

