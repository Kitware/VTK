//
// Define cell array
//
#ifndef __vlCellArray_h
#define __vlCellArray_h

#include "IntArray.hh"

class vlCellArray : public vlObject 
{
public:
  vlCellArray() : NumberOfCells(0), Location(0) {};
  int Initialize(const int sz, const int ext=1000) 
    {return this->Ia.Initialize(sz,ext);};
  vlCellArray (const int sz, const int ext=1000):NumberOfCells(0),Location(0),Ia(sz,ext){};
  vlCellArray(const vlCellArray& ca);
  ~vlCellArray() {};
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
  void InsertNextCell(int npts, int* pts)
  {
    int id = this->Ia.GetMaxId() + npts + 1;
    this->Ia.InsertValue(id,pts[npts-1]);
    this->Ia[id-npts] = npts;
    for (int i=0; i<npts-1; i++) this->Ia[id-npts+i+1] = pts[i];
    NumberOfCells++;
  }
  void InsertNextCell(int npts)
  {
    this->Location = this->Ia.InsertNextValue(npts) + 1;
    NumberOfCells++;
  }
  void InsertCellPoint(int id) 
    {this->Ia.InsertValue(this->Location++,id);};
  int GetNumberOfCells() {return NumberOfCells;};
  void InitTraversal() {this->Location=0;};
  void Squeeze() {this->Ia.Squeeze();};
  int EstimateSize(int numCells, int maxPtsPerCell) 
    {return numCells*(1+maxPtsPerCell);};
  int GetSize() {return Ia.GetSize();};
  void GetCell(int loc, int &npts, int* &pts)
    {npts=this->Ia.GetValue(loc++); pts=this->Ia.GetPtr(loc);};
  int GetLocation() {return this->Location;};
  
protected:
  int NumberOfCells;
  int Location;
  vlIntArray Ia;
};

#endif
