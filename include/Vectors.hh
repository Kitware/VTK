//
// Abstract interface to 3D vectors.
//
#ifndef __vlVectors_h
#define __vlVectors_h

#include "Object.hh"

class vlIdList;
class vlFloatVectors;

class vlVectors : public vlObject 
{
public:
  virtual ~vlVectors() {};
  virtual vlVectors *MakeObject(int sze, int ext=1000) = 0;
  virtual int NumVectors() = 0;
  virtual float *GetVector(int i) = 0;
  virtual void SetVector(int i,float x[3]) = 0;       // fast insert
  virtual void InsertVector(int i, float x[3]) = 0;   // allocates memory as necessary
  void GetVectors(vlIdList& ptId, vlFloatVectors& fp);
  char *GetClassName() {return "vlVectors";};
};

#endif
