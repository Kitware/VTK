//
// Abstract interface to texture coordinates
//
#ifndef __vlTCoords_h
#define __vlTCoords_h

#include "Object.hh"

class vlIdList;
class vlFloatTCoords;

class vlTCoords : public vlObject 
{
public:
  virtual ~vlTCoords() {};
  virtual vlTCoords *MakeObject(int sze, int d=2, int ext=1000) = 0;
  virtual int NumTCoords() = 0;
  virtual int GetDimension() = 0;
  virtual float *GetTCoord(int i) = 0;
  virtual void SetTCoord(int i,float *x) = 0;          // fast insert
  virtual void InsertTCoord(int i, float *x) = 0;      // allocates memory as necessary
  void GetTCoords(vlIdList& ptId, vlFloatTCoords& fp);
  char *GetClassName() {return "vlTCoords";};
};

#endif
