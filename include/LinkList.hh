/*=========================================================================

  Program:   Visualization Toolkit
  Module:    LinkList.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkLinkList - object represents upward pointers from points to list of cells using each point
// .SECTION Description
// vtkLinkList is a supplemental object to CellArray and CellList to allow
// access from points to cells using the points. LinkList is a collection 
// of Links, each link represents a dynamic list of cell id's using the 
// point. The information provided by this object can be used to determine 
// neighbors and construct other local topological information.

#ifndef __vtkLinkList_h
#define __vtkLinkList_h

#include "RefCount.hh"
class vtkDataSet;

struct _vtkLink_s {
    unsigned short ncells;
    int *cells;
};

class vtkLinkList : public vtkRefCount 
{
public:
  vtkLinkList():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  vtkLinkList(int sz, int ext=1000);
  ~vtkLinkList();
  char *GetClassName() {return "vtkLinkList";};

  _vtkLink_s &GetLink(int ptId);
  unsigned short GetNcells(int ptId);
  void BuildLinks(vtkDataSet *data);
  int *GetCells(int ptId);
  void InsertNextCellReference(int ptId, int cellId);

  void DeletePoint(int ptId);
  void RemoveCellReference(int cellId, int ptId);
  void ResizeCellList(int ptId, int size);

  void Squeeze();
  void Reset();

private:
  void IncrementLinkCount(int ptId);
  void AllocateLinks(int n);
  void InsertCellReference(int ptId, unsigned short pos, int cellId);

  _vtkLink_s *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  _vtkLink_s *Resize(int sz);  // function to resize data
};

// Description:
// Get a link structure given a point id.
inline _vtkLink_s &vtkLinkList::GetLink(int ptId) {return this->Array[ptId];};

// Description:
// Get the number of cells using this point.
inline unsigned short vtkLinkList::GetNcells(int ptId) 
{
  return this->Array[ptId].ncells;
}

// Description:
// Return a list of cell ids using the point.
inline int *vtkLinkList::GetCells(int ptId) {return this->Array[ptId].cells;};

// Description:
// Increment the count of the number of cells using the point.
inline void vtkLinkList::IncrementLinkCount(int ptId) 
{
  this->Array[ptId].ncells++;
}

// Description:
// Insert a cell id into the list of cells using the point.
inline void vtkLinkList::InsertCellReference(int ptId, unsigned short pos, int cellId) 
{
  this->Array[ptId].cells[pos] = cellId;
}

// Description:
// Delete point (and storage) by destroying links to using cells.
inline void vtkLinkList::DeletePoint(int ptId)
{
  this->Array[ptId].ncells = 0;
  delete [] this->Array[ptId].cells;
  this->Array[ptId].cells = NULL;
}

// Description:
// Insert a cell id into the list of cells (at the end) using the cell id 
// provided. (Make sure to extend the link list (if necessary) using the
// method ResizeCellList()).
inline void vtkLinkList::InsertNextCellReference(int ptId, int cellId) 
{
  this->Array[ptId].cells[this->Array[ptId].ncells++] = cellId;
}

// Description:
// Delete the reference to the cell (cellId) from the point (ptId). This
// removes the reference to the cellId from the cell list, but does not resize
// the list (recover memory with ResizeCellList(), if necessary).
inline void vtkLinkList::RemoveCellReference(int cellId, int ptId)
{
  int *cells=this->Array[ptId].cells;
  int ncells=this->Array[ptId].ncells;

  for (int i=0; i < ncells; i++)
    {
    if (cells[i] == cellId)
      {
      for (int j=i; j < (ncells-1); j++) cells[j] = cells[j+1];
      this->Array[ptId].ncells--;
      break;
      }
    }
}

// Description:
// Increase the length of the list of cells using a point by the size 
// specified. 
inline void vtkLinkList::ResizeCellList(int ptId, int size)
{
  int *cells, newSize;

  newSize = this->Array[ptId].ncells + size;
  cells = new int[newSize];
  memcpy(cells, this->Array[ptId].cells, this->Array[ptId].ncells*sizeof(int));
  delete [] this->Array[ptId].cells;
  this->Array[ptId].cells = cells;
}

#endif

