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
//
// Define link array
//
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
  vlLinkList():Array(0),Size(0),MaxId(-1),Extend(1000) {};
  vlLinkList(int sz, int ext=1000);
  ~vlLinkList();
  char *GetClassName() {return "vlLinkList";};
  vlLink &GetLink(int id) {return this->Array[id];};
  unsigned short GetNcells(int id) {return this->Array[id].ncells;};
  int *GetCells(int id) {return this->Array[id].cells;};
  void IncrementLinkCount(int id) {this->Array[id].ncells++;};
  void AllocateLinks();
  void InsertCellReference(int id, unsigned short pos, int cellId)
    {this->Array[id].cells[pos] = cellId;};
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

#endif
