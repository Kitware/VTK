//
// Floating point representation of 3D vectors.
//
//  use internal floating point array to represent data
//
#ifndef FloatVectors_h
#define FloatVectors_h

#include "Params.h"
#include "FArray.h"
#include "FTriple.h"
#include "IdList.h"

class FloatVectors : public RefCount {
public:
  FloatVectors();
  int Initialize(const int sz, const int ext=1000) {return v.Initialize(3*sz,3*ext);};
  FloatVectors(const FloatVectors& fp);
  FloatVectors(const int sz, const int ext);
  FloatVectors(FloatArray& fa);
  ~FloatVectors();
  int numVectors();
  void reset();
  FloatVectors &operator=(const FloatVectors& fp);
  void operator+=(const FloatVectors& fp);
  void operator+=(const FloatTriple& ft) {
    v += ft.x[0];
    v += ft.x[1];
    v += ft.x[2];
  }
  FloatTriple &operator[](const int i) {ft.x = v.getPtr(3*i); return ft;};
  void insertVector(const int i, FloatTriple &ft) {
      v.insertValue(3*i+2, ft.x[2]);
      v[3*i] =  ft.x[0];
      v[3*i+1] =  ft.x[1];
  }
  void insertVector(const int i, float *x) {
      v.insertValue(3*i+2, x[2]);
      v[3*i] =  x[0];
      v[3*i+1] =  x[1];
  }
  int insertNextVector(float *x) {
    int id = v.insertNextValue(x[0]);
    v.insertNextValue(x[1]);
    v.insertNextValue(x[2]);
    return id/3;
  }
  void getVectors(IdList& ptId, FloatVectors& fv);
  float *getArray();
private:
  FloatArray v;
  FloatTriple ft;
};

#endif
