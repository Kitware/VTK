//
// Define cell array
//
#ifndef __vlCellArray_h
#define __vlCellArray_h

#include "IntArray.hh"

class vlCellArray : public vlObject 
{
public:
  vlCellArray() : NumCells(0), Loc(0) {};
  int Initialize(const int sz, const int ext=1000) 
    {return this->Ia.Initialize(sz,ext);};
  vlCellArray (const int sz, const int ext=1000):NumCells(0),Loc(0),Ia(sz,ext){};
  ~vlCellArray() {};
  int GetNextCell(int& npts, int* &pts)
  {
    if ( this->Ia.GetMaxId() >= 0 && this->Loc <= this->Ia.GetMaxId() ) 
      {
      npts = this->Ia.GetValue(this->Loc++);
      pts = this->Ia.GetPtr(this->Loc);
      this->Loc += npts;
      return 1;
      }
    else
      {
      return 0;
      }
  }
  void InsertNextCell(int npts, int* pts)
  {
    int id = this->Ia.GetMaxId() + npts + 1;
    this->Ia.InsertValue(id,pts[npts-1]);
    this->Ia[id-npts] = npts;
    for (int i=0; i<npts-1; i++) this->Ia[id-npts+i+1] = pts[i];
    NumCells++;
  }
  int GetNumCells() {return NumCells;};
  void InitTraversal() {this->Loc=0;};
  void Squeeze() {this->Ia.Squeeze();};
  int EstimateSize(int numCells, int maxPtsPerCell) 
    {return numCells*(1+maxPtsPerCell);};
  
private:
  int NumCells;
  int Loc;
  vlIntArray Ia;
};

#endif
