/*=========================================================================

  Program:   Visualization Library
  Module:    CellArr.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Define cell array.  The cell array is a raw integer list representing many 
// cells.  The form of the list is: (n,id1,id2,...,idn, n,id1,id2,...,idn, ...)
// where n is the number of points in the cell, and id is a zero-offset index 
// into an associated point list.
//
// Advantages of this data structure are its compactness, simplicity, and easy 
// interface to external data.  However, it is totally inadequate for random 
// access.  This functionality (when necessary) is accomplished by using the 
// CellList and LinkList objects to extend the definition of data structure.
//
#ifndef __vlCellArray_h
#define __vlCellArray_h

#include "IntArray.hh"
#include "Cell.hh"

class vlCellArray : public vlObject 
{
public:
  vlCellArray() : NumberOfCells(0), Location(0) {};
  int Allocate(const int sz, const int ext=1000) 
    {return this->Ia.Allocate(sz,ext);};
  void Initialize() {return this->Ia.Initialize();};
  vlCellArray (const int sz, const int ext=1000):NumberOfCells(0),Location(0),Ia(sz,ext){};
  vlCellArray(const vlCellArray& ca);
  ~vlCellArray() {};
  char *GetClassName() {return "vlCellArray";};

  // return number of cells represented by this list
  int GetNumberOfCells() {return this->NumberOfCells;};

  // Methods to create cells from different inputs.
  // Create a cell by specifying the number of pts and an array of point id's
  void InsertNextCell(int npts, int* pts)
  {
    int id = this->Ia.GetMaxId() + npts + 1;
    this->Ia.InsertValue(id,pts[npts-1]);
    this->Ia[id-npts] = npts;
    for (int i=0; i<npts-1; i++) this->Ia[id-npts+i+1] = pts[i];
    this->NumberOfCells++;
    this->Location += npts + 1;
  }

  // Create cells by specifying count, and then adding points one at a time using
  // method InsertCellPoint()
  void InsertNextCell(int npts)
  {
    this->Location = this->Ia.InsertNextValue(npts) + 1;
    this->NumberOfCells++;
  }
  void InsertCellPoint(int id) 
    {this->Ia.InsertValue(this->Location++,id);};

  // Create a cell from a cell object
  void InsertNextCell(vlCell *cell)
  {
    int npts = cell->GetNumberOfPoints();
    int id = this->Ia.GetMaxId() + npts + 1;
    this->Ia.InsertValue(id,cell->PointIds.GetId(npts-1));
    this->Ia[id-npts] = npts;
    for (int i=0; i<npts-1; i++) this->Ia[id-npts+i+1] = cell->PointIds.GetId(i);
    this->NumberOfCells++;
    this->Location += npts + 1;
  }

  // utility routines to help manage memory of cell data structure. EstimateSize()
  // returns a value used to initialize cell array based on number of cells and 
  // maximum number of points making up cell.  If every cell is the same size (in 
  // terms of number of points) then the estimate is guaranteed exact.
  // The Squeeze() function recovers memory from estimated size.
  int EstimateSize(int numCells, int maxPtsPerCell) 
    {return numCells*(1+maxPtsPerCell);};
  void Squeeze() {this->Ia.Squeeze();};

  // These are cell traversal methods.  More efficient than DataSet traversal
  // methods.  Both traversal methods can be used together, if necessary.  
  // Cell traversal methods are initialized with InitTraveral() and then 
  // followed by repeated calls to GetNextCell()
  void InitTraversal() {this->Location=0;};
  int GetNextCell(int& npts, int* &pts)
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

  // access methods for building data structures
  int GetSize() {return Ia.GetSize();};
  void GetCell(int loc, int &npts, int* &pts)
    {npts=this->Ia.GetValue(loc++); pts=this->Ia.GetPtr(loc);};

  // Use only in conjunction with cell traversal methods
  int GetLocation(int npts) {return (this->Location - npts - 1);};
  
  // Special method flips ordering of current cell.  Works in conjunction with
  // cell traversal methods.
  void ReverseCell(int loc)
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
  void ReplaceCell(int loc, vlIdList& ptIds)
  {
    int i;
    int npts=this->Ia.GetValue(loc);
    int *pts=this->Ia.GetPtr(loc+1);
    for (i=0; i < npts; i++)  pts[i] = ptIds.GetId(i);
  }

protected:
  int NumberOfCells;
  int Location;
  vlIntArray Ia;
};

#endif
