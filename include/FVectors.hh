//
// Floating point representation of 3D vectors.
//
//  use internal floating point array to represent data
//
#ifndef __vlFloatVectors_h
#define __vlFloatVectors_h

#include "Object.h"
#include "FArray.h"
#include "FTriple.h"
#include "IdList.h"

class vlFloatVectors : public vlObject {
public:
  vlFloatVectors();
  int Initialize(const int sz, const int ext=1000) {return this->V.Initialize(3*sz,3*ext);};
  vlFloatVectors(const vlFloatVectors& fp);
  vlFloatVectors(const int sz, const int ext);
  vlFloatVectors(vlFloatArray& fa);
  virtual ~vlFloatVectors();
  int NumVectors();
  void Reset();
  vlFloatVectors &operator=(const vlFloatVectors& fp);
  void operator+=(const vlFloatVectors& fp);
  void operator+=(const vlFloatTriple& ft) {
    this->V += ft.X[0];
    this->V += ft.X[1];
    this->V += ft.X[2];
  }
  vlFloatTriple &operator[](const int i) 
    {this->Ft.X = this->V.GetPtr(3*i); return this->Ft;};
  void insertVector(const int i, vlFloatTriple &ft) {
      this->V.InsertValue(3*i+2, Ft.X[2]);
      this->V[3*i] =  Ft.X[0];
      this->V[3*i+1] =  Ft.X[1];
  }
  void InsertVector(const int i, float *x) {
      this->V.InsertValue(3*i+2, x[2]);
      this->V[3*i] =  x[0];
      this->V[3*i+1] =  x[1];
  }
  int InsertNextVector(float *x) {
    int id = this->V.InsertNextValue(x[0]);
    this->V.InsertNextValue(x[1]);
    this->V.InsertNextValue(x[2]);
    return id/3;
  }
  void GetVectors(vlIdList& ptId, vlFloatVectors& fv);
  float *GetArray();
private:
  vlFloatArray V;
  vlFloatTriple Ft;
};

#endif
