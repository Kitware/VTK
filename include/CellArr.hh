//
// Define cell array
//
#ifndef __vlCellArray_h
#define __vlCellArray_h

#include "IntArray.hh"
#include "Cell.hh"

class vlCellArray : public vlObject 
{
public:
  vlCellArray() : NumberOfCells(0), Location(0) {};
  int Initialize(const int sz, const int ext=1000) 
    {return this->Ia.Initialize(sz,ext);};
  vlCellArray (const int sz, const int ext=1000):NumberOfCells(0),Location(0),Ia(sz,ext){};
  vlCellArray(const vlCellArray& ca);
  ~vlCellArray() {};

  int GetNumberOfCells() {return this->NumberOfCells;};

  // utility routines to help manage memory of cell data structure. EstimateSize()
  // returns a value used to initialize cell array based on number of cells and 
  // maximum number of points making up cell.  If every cell is the same size (in 
  // terms of number of points) then the estimate is guaranteed exact.
  // The Squeeze() function recovers memory from estimated size.
  int EstimateSize(int numCells, int maxPtsPerCell) 
    {return numCells*(1+maxPtsPerCell);};
  void Squeeze() {this->Ia.Squeeze();};

  // efficient method of traversing cell array.  Use InitTraveral() and then repeated
  // calls to GetNextCell()
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

  // create a cell by specifying the number of pts and an array of point id's
  void InsertNextCell(int npts, int* pts)
  {
    int id = this->Ia.GetMaxId() + npts + 1;
    this->Ia.InsertValue(id,pts[npts-1]);
    this->Ia[id-npts] = npts;
    for (int i=0; i<npts-1; i++) this->Ia[id-npts+i+1] = pts[i];
    NumberOfCells++;
  }

  // create cells by specifying count, and then adding points one at a time using
  // method InsertCellPoint()
  void InsertNextCell(int npts)
  {
    this->Location = this->Ia.InsertNextValue(npts) + 1;
    NumberOfCells++;
  }
  void InsertCellPoint(int id) 
    {this->Ia.InsertValue(this->Location++,id);};

  // create a cell from a cell object
  void InsertNextCell(vlCell *cell)
  {
    int npts = cell->GetNumberOfPoints();
    int id = this->Ia.GetMaxId() + npts + 1;
    this->Ia.InsertValue(id,cell->PointIds.GetId(npts-1));
    this->Ia[id-npts] = npts;
    for (int i=0; i<npts-1; i++) this->Ia[id-npts+i+1] = cell->PointIds.GetId(i);
    NumberOfCells++;
  }

  // access methods for building data structures
  int GetSize() {return Ia.GetSize();};
  void GetCell(int loc, int &npts, int* &pts)
    {npts=this->Ia.GetValue(loc++); pts=this->Ia.GetPtr(loc);};
  int GetLocation(int npts) {return (this->Location - npts - 1);};
  
protected:
  int NumberOfCells;
  int Location;
  vlIntArray Ia;
};

#endif
