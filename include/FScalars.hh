//
// Floating point representation of scalars
//
//  use internal floating point array to represent data
//
#ifndef __vlFloatScalars_h
#define __vlFloatScalars_h

#include "Object.hh"
#include "FArray.hh"
#include "FTriple.hh"
#include "IdList.hh"

class vlFloatScalars : public vlObject 
{
public:
  vlFloatScalars() {};
  int Initialize(const int sz, const int ext=1000) 
    {return S.Initialize(sz,ext);};
  vlFloatScalars(const vlFloatScalars& fs) {this->S = fs.S;};
  vlFloatScalars(const int sz, const int ext):S(sz,ext){};
  ~vlFloatScalars() {};
  char *GetClassName() {return "vlFloatScalars";};
  int NumScalars() {return (this->S.GetMaxId()+1);};
  void Reset() {this->S.Reset();};
  vlFloatScalars &operator=(const vlFloatScalars& fs);
  void operator+=(const vlFloatScalars& fs) {this->S += fs.S;};
  void operator+=(const float f) {this->S += f;};
  float &operator[](const int i) {return this->S[i];};
  void InsertScalar(const int i, float f) {S.InsertValue(i, f);};
  int InsertNextScalar(float f) {return S.InsertNextValue(f);};
  void GetScalars(vlIdList& ptId, vlFloatScalars& fs);
  float *GetArray() {return this->S.GetArray();};
private:
  vlFloatArray S;
};

#endif
