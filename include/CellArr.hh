//
// Define cell array
//
#ifndef CellArray_h
#define CellArray_h

#include "Params.h"
#include "IntArray.h"

class CellArray : public RefCount {
public:
  CellArray() : ncells(0), loc(0) {};
  int Initialize(const int sz, const int ext=1000) {return ia.Initialize(sz,ext);};
  CellArray (const int sz, const int ext=1000):ncells(0),loc(0),ia(sz,ext){};
  ~CellArray() {};
  int getNextCell(int& npts, int* pts)
  {
    if ( loc <= ia.getMaxId() ) 
    {
      npts = ia.getValue(loc++);
      pts = ia.getPtr(loc);
      loc += npts;
      return 1;
    }
    else
    {
      return 0;
    }
  }
  void insertNextCell(int npts, int* pts)
  {
    ncells++;
    ia.insertNextValue(npts);
    for (int i=0; i<npts; i++) ia.insertNextValue(pts[i]);
  }
  int numCells() {return ncells;};
  void initTraversal() {loc=0;};
  void squeeze() {ia.squeeze();};
  
private:
  int ncells;
  int loc;
  IntArray ia;
};

#endif
