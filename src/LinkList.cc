/*=========================================================================

  Program:   Visualization Toolkit
  Module:    LinkList.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "LinkList.hh"
#include "DataSet.hh"

vtkLinkList::vtkLinkList(int sz, int ext)
{
  static _vtkLink_s linkInit = {0,NULL};

  this->Size = sz;
  this->Array = new _vtkLink_s[sz];
  this->Extend = ext;
  this->MaxId = -1;

  for (int i=0; i < sz; i++) this->Array[i] = linkInit;
}

vtkLinkList::~vtkLinkList()
{
  if ( this->Array == NULL ) return;

  for (int i=0; i<=this->MaxId; i++)
    {
    if ( this->Array[i].cells != NULL ) 
      delete [] this->Array[i].cells;
    }

  delete [] this->Array;
}

// Description:
// Allocate memory for the list of lists of cell ids.
void vtkLinkList::AllocateLinks(int n)
{
  for (int i=0; i < n; i++)
    {
    this->Array[i].cells = new int[this->Array[i].ncells];
    }
}

// Description:
// Reclaim any unused memory.
void vtkLinkList::Squeeze()
{
  this->Resize (this->MaxId+1);
}


void vtkLinkList::Reset()
{
  this->MaxId = -1;
}
//
// Private function does "reallocate"
//
_vtkLink_s *vtkLinkList::Resize(int sz)
{
  int i;
  _vtkLink_s *newArray;
  int newSize;

  if ( sz >= this->Size ) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  newArray = new _vtkLink_s[newSize];

  for (i=0; i<sz && i<this->Size; i++)
    newArray[i] = this->Array[i];

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

// Description:
// Build the link list array.
void vtkLinkList::BuildLinks(vtkDataSet *data)
{
  int numPts = data->GetNumberOfPoints();
  int numCells = data->GetNumberOfCells();
  int i, j, ptId, cellId;
  vtkCell *cell;
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
  this->MaxId = numPts - 1;

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

