/*=========================================================================

  Program:   Visualization Library
  Module:    LinkList.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "LinkList.hh"
#include "DataSet.hh"

vlLinkList::vlLinkList(int sz, int ext)
{
  static vlLink linkInit = {0,0};

  this->Size = sz;
  this->Array = new vlLink[sz];
  this->Extend = ext;
  this->MaxId = -1;

  for (int i=0; i < sz; i++) this->Array[i] = linkInit;
}

vlLinkList::~vlLinkList()
{
  delete [] this->Array;
}

// Description:
// Allocate memory for the list of lists of cell ids.
void vlLinkList::AllocateLinks(int n)
{
  for (int i=0; i < n; i++)
    {
    this->Array[i].cells = new int[this->Array[i].ncells];
    }
}

// Description:
// Reclaim any unused memory.
void vlLinkList::Squeeze()
{
  this->Resize (this->MaxId+1);
}


void vlLinkList::Reset()
{
  this->MaxId = -1;
}
//
// Private function does "reallocate"
//
vlLink *vlLinkList::Resize(int sz)
{
  int i;
  vlLink *newArray;
  int newSize;

  if ( sz >= this->Size ) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  newArray = new vlLink[newSize];

  for (i=0; i<sz && i<this->Size; i++)
    newArray[i] = this->Array[i];

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

// Description:
// Build the link list array.
void vlLinkList::BuildLinks(vlDataSet *data)
{
  int numPts = data->GetNumberOfPoints();
  int numCells = data->GetNumberOfCells();
  int i, j, ptId, cellId;
  vlCell *cell;
  unsigned short *linkLoc;

  // traverse data to determine number of uses of each point
  for (cellId=0; cellId < numCells; cellId++)
    {
    cell = data->GetCell(cellId);
    for (j=0; j < cell->GetNumberOfPoints(); j++)
      {
      this->IncrementLinkCount(cell->PointIds.GetId(j));      
      }      
    }

  // now allocate storage for the links
   this->AllocateLinks(numPts);

  // fill out lists with references to cells
  linkLoc = new unsigned short[numPts];
  for (i=0; i < numPts; i++) linkLoc[i] = 0;

  for (cellId=0; cellId < numCells; cellId++)
    {
    cell = data->GetCell(cellId);
    for (j=0; j < cell->GetNumberOfPoints(); j++)
      {
      ptId = cell->PointIds.GetId(j);
      this->InsertCellReference(ptId, (linkLoc[ptId])++, cellId);      
      }      
    }

  delete [] linkLoc;
}

