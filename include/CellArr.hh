/*=========================================================================

  Program:   Visualization Library
  Module:    CellArr.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlCellArray - object represents cell connectivity
// .SECTION Description
// vlCellArray is a supporting object that explicitly represents cell 
// connectivity. The cell array structure is a raw integer list
// of the form: (n,id1,id2,...,idn, n,id1,id2,...,idn, ...)
// where n is the number of points in the cell, and id is a zero-offset index 
// into an associated point list.
//    Advantages of this data structure are its compactness, simplicity, and 
// easy interface to external data.  However, it is totally inadequate for 
// random access.  This functionality (when necessary) is accomplished by 
// using the vlCellList and vlLinkList objects to extend the definition of 
// the data structure.

#ifndef __vlCellArray_h
#define __vlCellArray_h

#include "IntArray.hh"
#include "Cell.hh"

class vlCellArray : public vlRefCount
{
public:
  vlCellArray() : NumberOfCells(0), Location(0) {};
  int Allocate(const int sz, const int ext=1000) 
    {return this->Ia.Allocate(sz,ext);};
  void Initialize() {this->Ia.Initialize();};
  vlCellArray (const int sz, const int ext=1000):NumberOfCells(0),Location(0),Ia(sz,ext){};
  vlCellArray(const vlCellArray& ca);
  ~vlCellArray() {};
  char *GetClassName() {return "vlCellArray";};

  int GetNumberOfCells();
  void InsertNextCell(int npts, int* pts);
  void InsertNextCell(int npts);
  void InsertCellPoint(int id);
  void InsertNextCell(vlCell *cell);

  int EstimateSize(int numCells, int maxPtsPerCell);
  void Squeeze();

  void InitTraversal();
  int GetNextCell(int& npts, int* &pts);

  int GetSize();
  void GetCell(int loc, int &npts, int* &pts);

  int GetLocation(int npts);
  
  void ReverseCell(int loc);
  void ReplaceCell(int loc, vlIdList& ptIds);

protected:
  int NumberOfCells;
  int Location;
  vlIntArray Ia;
};

// Description:
// Get the number of cells in the array.
inline int vlCellArray::GetNumberOfCells() {return this->NumberOfCells;};

// Description:
// Create a cell by specifying the number of pts and an array of point id's
inline void vlCellArray::InsertNextCell(int npts, int* pts)
{
  int id = this->Ia.GetMaxId() + npts + 1;
  this->Ia.InsertValue(id,pts[npts-1]);
  this->Ia[id-npts] = npts;
  for (int i=0; i<npts-1; i++) this->Ia[id-npts+i+1] = pts[i];
  this->NumberOfCells++;
  this->Location += npts + 1;
}

// Description:
// Create cells by specifying count, and then adding points one at a time using
// method InsertCellPoint(). WARNING: it is the user's responsibility not to
// exceed the maximum allowable points per cell (MAX_CELL_SIZE).
inline void vlCellArray::InsertNextCell(int npts)
{
  this->Location = this->Ia.InsertNextValue(npts) + 1;
  this->NumberOfCells++;
}

// Description:
// Used in conjunction with InsertNextCell(int npts) to add another point
// to the list of cells.
inline void vlCellArray::InsertCellPoint(int id) 
{
  this->Ia.InsertValue(this->Location++,id);
}

// Description:
// Insert a cell object.
inline void vlCellArray::InsertNextCell(vlCell *cell)
{
  int npts = cell->GetNumberOfPoints();
  int id = this->Ia.GetMaxId() + npts + 1;
  this->Ia.InsertValue(id,cell->PointIds.GetId(npts-1));
  this->Ia[id-npts] = npts;
  for (int i=0; i<npts-1; i++) this->Ia[id-npts+i+1] = cell->PointIds.GetId(i);
  this->NumberOfCells++;
  this->Location += npts + 1;
}

// Description:
// Utility routines helps manage memory of cell array. EstimateSize()
// returns a value used to initialize and allocate memory for array based
// on number of cells and maximum number of points making up cell.  If 
// every cell is the same size (in terms of number of points) then the 
// memory estimate is guaranteed exact. (If not exact, use Squeeze() to
// reclaim any extra memory).
inline int vlCellArray::EstimateSize(int numCells, int maxPtsPerCell) 
{
  return numCells*(1+maxPtsPerCell);
}

// Description:
// Reclaim any extra memory.
inline void vlCellArray::Squeeze() {this->Ia.Squeeze();}

// Description:
// Cell traversal methods that are more efficient than vlDataSet traversal
// methods.  InitTraversal() initializes the traversal of the list of cells.
inline void vlCellArray::InitTraversal() {this->Location=0;}

// Description:
// Cell traversal methods that are more efficient than vlDataSet traversal
// methods.  GetNextCell() gets the next cell in the list. If end of list
// is encountered, 0 is returned.
inline int vlCellArray::GetNextCell(int& npts, int* &pts)
{
  if ( this->Ia.GetMaxId() >= 0 && this->Location <= this->Ia.GetMaxId() ) 
    {
    npts = this->Ia.GetValue(this->Location++);
    pts = this->Ia.GetPtr(this->Location);
    this->Location += npts;
    return 1;
    }
  else
    {
    return 0;
    }
}

// Description:
// Get the size of the allocated data.
inline int vlCellArray::GetSize() {return Ia.GetSize();};

// Description:
// Internal method used to retrieve a cell given an offset into
// the internal array.
inline void vlCellArray::GetCell(int loc, int &npts, int* &pts)
{
  npts=this->Ia.GetValue(loc++); pts=this->Ia.GetPtr(loc);
}

// Description:
// Computes the current location within the internal array. Used in conjunction
// with GetCell(int loc,...).
inline int vlCellArray::GetLocation(int npts) {
  return (this->Location - npts - 1);
}

// Description:
// Special method inverts ordering of current cell. Must be called carefully or
// the cell topology may be corrupted.
inline void vlCellArray::ReverseCell(int loc)
{
  int i, tmp;
  int npts=this->Ia.GetValue(loc);
  int *pts=this->Ia.GetPtr(loc+1);
  for (i=0; i < (npts/2); i++) 
    {
    tmp = pts[i];
    pts[i] = pts[npts-i-1];
    pts[npts-i-1] = tmp;
    }
}

// Description:
// Replace the point ids of the cell with a different list of point ids.
inline void vlCellArray::ReplaceCell(int loc, vlIdList& ptIds)
{
  int i;
  int npts=this->Ia.GetValue(loc);
  int *pts=this->Ia.GetPtr(loc+1);
  for (i=0; i < npts; i++)  pts[i] = ptIds.GetId(i);
}

#endif
