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
#include <iostream.h>
#include "CellList.hh"

vlCellList::vlCellList(const int sz, const int ext)
{
  this->Size = sz;
  this->Array = new vlCell[sz];
  this->Extend = ext;
  this->MaxId = -1;
}

vlCellList::~vlCellList()
{
  delete [] this->Array;
}

//
// Add a cell to structure
//
void vlCellList::InsertCell(const int id, const short type, const int loc)
{
  vlCell *cell;

  if ( id >= this->Size ) this->Resize(id);
  if ( id > this->MaxId ) this->MaxId = id;

  cell = this->Array + id;
  cell->type = type;
  cell->loc = loc;

  return;
}

int vlCellList::InsertNextCell(const short type, const int loc)
{
  this->InsertCell (++this->MaxId,type,loc);
  return this->MaxId;
}

void vlCellList::Squeeze()
{
  this->Resize (this->MaxId+1);
}

void vlCellList::Reset()
{
  this->MaxId = -1;
}
//
// Private function does "reallocate"
//
vlCell *vlCellList::Resize(const int sz)
{
  int i;
  vlCell *newArray;
  int newSize;

  if ( sz >= this->Size )  newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  if ( (newArray = new vlCell[newSize]) == 0 )
  {
    vlErrorMacro(<< "Cannot allocate memory\n");
    return 0;
  }

  for (i=0; i<sz && i<this->Size; i++)
      newArray[i] = this->Array[i];

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

