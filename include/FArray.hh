//
//  Dynamic, self adjusting floating point array
//
#ifndef FloatArray_h
#define FloatArray_h

#include "Params.h"
#include "RefCount.h"

class FloatArray : public RefCount {

public:
  FloatArray():array(0),size(0),max_id(-1),extend(1000) {};
  int Initialize(const int sz, const int ext=1000);
  FloatArray(const int sz, const int ext=1000);
  FloatArray(const FloatArray& fa);
  ~FloatArray();
  float getValue(const int id) {return array[id];};
  float *getPtr(const int id) {return array + id;};
  FloatArray &insertValue(const int id, const float f);
  int insertNextValue(const float f);
  FloatArray &operator=(const FloatArray& fa);
  FloatArray &operator+=(const FloatArray& fa);
  void operator+=(const float f) {insertNextValue(f);};
  // operator[] can be used on both left and right side of expression;
  // Note: if used on left hand side, user's responsibility to do range checking
  float& operator[](const int i) {return array[i];};
  void squeeze();
  int getSize();
  int getMaxId();
  void setMaxId(int id);
  float *getArray();
  void reset();

private:
  float *array;   // pointer to data
  int size;       // allocated size of data
  int max_id;     // maximum index inserted thus far
  int extend;     // grow array by this point
  float *resize(const int sz);  // function to resize data
};

#endif
