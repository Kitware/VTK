//
// Abstract interface to scalar values.
//
#ifndef __vlScalars_h
#define __vlScalars_h

#include "Object.hh"

class vlIdList;
class vlFloatScalars;

class vlScalars : public vlObject 
{
public:
  virtual ~vlScalars() {};
  virtual vlScalars *MakeObject(int sze, int ext=1000) = 0;
  virtual int NumScalars() = 0;
  virtual float GetScalar(int i) = 0;
  virtual void SetScalar(int i, float s) = 0;     // fast insert
  virtual void InsertScalar(int i, float s) = 0;  // allocates memory as necessary
  void GetScalars(vlIdList& ptId, vlFloatScalars& fp);
  char *GetClassName() {return "vlScalars";};
  virtual void ComputeRange();
  float *GetRange();

private:
  float Range[2];
  vlTimeStamp ComputeTime; // Time at which range computed
};

#endif
