//
// Floating point representation of scalars
//
//  use internal floating point array to represent data
//
#ifndef FloatScalars_h
#define FloatScalars_h

#include "Params.h"
#include "FArray.h"
#include "FTriple.h"
#include "IdList.h"

class FloatScalars : public RefCount {
public:
  FloatScalars();
  int Initialize(const int sz, const int ext=1000) {return s.Initialize(sz,ext);};
  FloatScalars(const FloatScalars& fs);
  FloatScalars(const int sz, const int ext);
  FloatScalars(FloatArray& fa);
  ~FloatScalars();
  int numScalars();
  void reset();
  FloatScalars &operator=(const FloatScalars& fs);
  void operator+=(const FloatScalars& fs);
  void operator+=(const float f) {s += f;};
  float &operator[](const int i) {return s[i];};
  void insertScalar(const int i, float f) {s.insertValue(i, f);};
  int insertNextScalar(float f) {return s.insertNextValue(f);};
  void getScalars(IdList& ptId, FloatScalars& fs);
  float *getArray();
private:
  FloatArray s;
};

#endif
