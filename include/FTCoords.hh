//
// Floating point representation of 3D TCoords.
//
//  use internal floating point array to represent data
//
#ifndef FloatTCoords_h
#define FloatTCoords_h

#include "Params.h"
#include "FArray.h"
#include "FTriple.h"
#include "IdList.h"

class FloatTCoords : public RefCount {
public:
  FloatTCoords();
  int Initialize(const int sz, const int dim=2, const int ext=1000) {return tc.Initialize(dim*sz,dim*ext);};
  FloatTCoords(const FloatTCoords& tc);
  FloatTCoords(const int sz, const int d=2, const int ext=1000);
  FloatTCoords(FloatArray& fa);
  ~FloatTCoords();
  int numTCoords();
  void reset();
  FloatTCoords &operator=(const FloatTCoords& ftc);
  void operator+=(const FloatTCoords& ftc);
  void operator+=(const FloatTriple& ft) {
    for(int j=0; j<dim; j++) tc += ft.x[j];
  }
  FloatTriple &operator[](const int i) {ft.x = tc.getPtr(3*i); return ft;};
  void insertTCoord(const int i, FloatTriple &ft) {
    for(int j=0; j<dim; j++) tc.insertValue(3*i+j, ft.x[j]);
  }
  void insertTCoord(const int i, float *x) {
    for(int j=0; j<dim; j++) tc.insertValue(3*i+j, x[j]);
  }
  int insertNextTCoord(float *x) {
    int id = tc.insertNextValue(x[0]);
    for(int j=1; j<dim; j++) tc.insertNextValue(x[j]);
    return id/dim;
  }
  FloatTCoords& setDimension(const int i) {
    dim = ( (i<1) ? 1 : (i>3) ? 3 : i);
    return *this;
  }
  int getDimension() {return dim;};
  void getTCoords(IdList& ptId, FloatTCoords& ftc);
  float *getArray();
private:
  FloatArray tc;
  FloatTriple ft;
  int dim;
};

#endif
