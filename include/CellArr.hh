//
// Define cell array
//
#ifndef __vlCellArray_h
#define __vlCellArray_h

#include "IntArray.h"

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
    if ( this->Loc <= this->Ia.GetMaxId() ) 
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
    NumCells++;
    this->Ia.InsertNextValue(npts);
    for (int i=0; i<npts; i++) this->Ia.InsertNextValue(pts[i]);
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
