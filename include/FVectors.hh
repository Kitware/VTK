//
// Floating point representation of 3D vectors.
//
//  use internal floating point array to represent data
//
#ifndef __vlFloatVectors_h
#define __vlFloatVectors_h

#include "Object.hh"
#include "FArray.hh"
#include "FTriple.hh"
#include "IdList.hh"

class vlFloatVectors : public vlObject 
{
public:
  vlFloatVectors() {};
  int Initialize(const int sz, const int ext=1000) 
    {return this->V.Initialize(3*sz,3*ext);};
  vlFloatVectors(const vlFloatVectors& fv) {this->V = fv.V;};
  vlFloatVectors(const int sz, const int ext=1000):V(3*sz,3*ext){};
  ~vlFloatVectors() {};
  char *GetClassName() {return "vlFloatVectors";};
  int NumVectors() {return (V.GetMaxId()+1)/3;};
  void Reset() {this->V.Reset();};
  vlFloatVectors &operator=(const vlFloatVectors& fv);
  void operator+=(const vlFloatVectors& fv){this->V += fv.V;};
  void operator+=(const vlFloatTriple& ft) {
    this->V += ft.X[0];
    this->V += ft.X[1];
    this->V += ft.X[2];
  }
  vlFloatTriple &operator[](const int i) 
    {this->Ft.X = this->V.GetPtr(3*i); return this->Ft;};
  void insertVector(const int i, vlFloatTriple &ft) {
      this->V.InsertValue(3*i+2, ft.X[2]);
      this->V[3*i] =  ft.X[0];
      this->V[3*i+1] =  ft.X[1];
  }
  void InsertVector(const int i, float *x) {
      this->V.InsertValue(3*i+2, x[2]);
      this->V[3*i] =  x[0];
      this->V[3*i+1] =  x[1];
  }
  int InsertNextVector(float *x) {
    int id = this->V.GetMaxId() + 3;
    this->V.InsertValue(id,x[2]);
    this->V[id-2] = x[0];
    this->V[id-1] = x[1];
    return id/3;
  }
  int InsertNextVector(vlFloatTriple &ft) {
    int id = this->V.GetMaxId() + 3;
    this->V.InsertValue(id,ft.X[2]);
    this->V[id-2] = ft.X[0];
    this->V[id-1] = ft.X[1];
    return id/3;
  }
  void GetVectors(vlIdList& ptId, vlFloatVectors& fv);
  float *GetArray() {return this->V.GetArray();};
private:
  vlFloatArray V;
  vlFloatTriple Ft;
};

#endif
