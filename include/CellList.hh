/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CellList.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkCellList - object provides direct access to cells in vtkCellArray
// .SECTION Description
// Supplemental object to vtkCellArray to allow random access into cells.
// The "location" field is the location in the vtkCellArray list in terms of an 
// integer offset.  An integer offset was used instead of a pointer for easy 
// storage and inter-process communication.

#ifndef __vtkCellList_h
#define __vtkCellList_h

#include "RefCount.hh"
#include "CellType.hh"

struct _vtkCell_s {
    unsigned char type; //from CellTypes.hh
    int loc; //location in associated CellArray object
};

class vtkCellList : public vtkRefCount 
{
public:
  vtkCellList() : Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  vtkCellList(const int sz, const int ext);
  ~vtkCellList();
  char *GetClassName() {return "vtkCellList";};

  _vtkCell_s &GetCell(const int id);
  unsigned char GetCellType(const int id);
  int GetCellLocation(const int id);
  void InsertCell(const int id, const unsigned char type, const int loc);
  int InsertNextCell(const unsigned char type, const int loc);

  void DeleteCell(int cellId);

  void Squeeze();
  void Reset();

private:
  _vtkCell_s *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  _vtkCell_s *Resize(const int sz);  // function to resize data
};

// Description:
// Return a reference to a cell list structure.
inline _vtkCell_s &vtkCellList::GetCell(const int id) 
{
  return this->Array[id];
}

// Description:
// Return the type of cell.
inline unsigned char vtkCellList::GetCellType(const int cellId) 
{
  return this->Array[cellId].type;
}

// Description:
// Return the location of the cell in the associated vtkCellArray.
inline int vtkCellList::GetCellLocation(const int cellId) 
{
  return this->Array[cellId].loc;
}

// Description:
// Delete cell by setting to NULL cell type.
inline void vtkCellList::DeleteCell(int cellId)
{
  this->Array[cellId].type = vtkNULL_ELEMENT;
}

#endif
