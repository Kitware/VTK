/*=========================================================================

  Program:   Visualization Library
  Module:    LinkList.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlLinkList - object represents upward pointers from point list to cell list
// .SECTION Description
// vlLinkList is a supplemental object to CellArray and CellList to allow
// access from points to cells using the points. LinkList is a collection 
// of Links, each link represents a dynamic list of cell id's using the 
// point. The information provided by this object can be used to determine 
// neighbors and construct other local topological information.

#ifndef __vlLinkList_h
#define __vlLinkList_h

#include "Object.hh"
class vlDataSet;

struct vlLink {
    unsigned short ncells;
    int *cells;
};

class vlLinkList : public vlObject {
public:
  vlLinkList():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  vlLinkList(int sz, int ext=1000);
  ~vlLinkList();
  char *GetClassName() {return "vlLinkList";};

  vlLink &GetLink(int ptId);
  unsigned short GetNcells(int ptId);
  int *GetCells(int ptId);
  void IncrementLinkCount(int ptId);
  void AllocateLinks(int n);
  void InsertCellReference(int ptId, unsigned short pos, int cellId);
  void BuildLinks(vlDataSet *data);

  void Squeeze();
  void Reset();

private:
  vlLink *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  vlLink *Resize(int sz);  // function to resize data
};

// Description:
// Get a link structure given a point id.
inline vlLink &vlLinkList::GetLink(int ptId) {return this->Array[ptId];};

// Description:
// Get the number of cells using this point.
inline unsigned short vlLinkList::GetNcells(int ptId) 
{
  return this->Array[ptId].ncells;
}

// Description:
// Return a list of cell ids using the point.
inline int *vlLinkList::GetCells(int ptId) {return this->Array[ptId].cells;};

// Description:
// Increment the count of the number of cells using the point.
inline void vlLinkList::IncrementLinkCount(int ptId) 
{
  this->Array[ptId].ncells++;
}

// Description:
// Insert a cell id into the list of cells using the point.
inline void vlLinkList::InsertCellReference(int ptId, unsigned short pos, int cellId) 
{
  this->Array[ptId].cells[pos] = cellId;
}

#endif

