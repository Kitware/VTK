//
// Floating point representation of 3D normals.
//
//  use internal floating point array to represent data
//
#ifndef FloatNormals_h
#define FloatNormals_h

#include "Params.h"
#include "FArray.h"
#include "FTriple.h"
#include "IdList.h"

class FloatNormals : public RefCount {
public:
  FloatNormals();
  int Initialize(const int sz, const int ext=1000) {return n.Initialize(3*sz,3*ext);};
  FloatNormals(const FloatNormals& fn);
  FloatNormals(const int sz, const int ext=1000);
  FloatNormals(FloatArray& fa);
  ~FloatNormals();
  int numNormals();
  void reset();
  FloatNormals &operator=(const FloatNormals& fn);
  void operator+=(const FloatNormals& fn);
  void operator+=(const FloatTriple& ft) {
    n += ft.x[0];
    n += ft.x[1];
    n += ft.x[2];
  }
  FloatTriple &operator[](const int i) {ft.x = n.getPtr(3*i); return ft;};
  void insertNormal(const int i, FloatTriple &ft) {
      n.insertValue(3*i+2, ft.x[2]);
      n[3*i] =  ft.x[0];
      n[3*i+1] =  ft.x[1];
  }
  void insertNormal(const int i, float *x) {
      n.insertValue(3*i+2, x[2]);
      n[3*i] =  x[0];
      n[3*i+1] =  x[1];
  }
  int insertNextNormal(float *x) {
    int id = n.insertNextValue(x[0]);
    n.insertNextValue(x[1]);
    n.insertNextValue(x[2]);
    return id/3;
  }
  void getNormals(IdList& ptId, FloatNormals& fn);
  float *getArray();
private:
  FloatArray n;
  FloatTriple ft;
};

#endif
