//
//  Dynamic, self adjusting integer array
//
#ifndef IntArray_h
#define IntArray_h

#include "Params.h"
#include "RefCount.h"

class IntArray : public RefCount {

public:
  IntArray():array(0),size(0),max_id(-1),extend(1000) {};
  int Initialize(const int sz, const int ext);
  IntArray(const int sz, const int ext);
  IntArray(const IntArray& ia);
  ~IntArray();
  int getValue(const int id) {return array[id];};
  int *getPtr(const int id) {return array + id;};
  IntArray &insertValue(const int id, const int i);
  int insertNextValue(const int i);
  IntArray &operator=(IntArray& ia);
  void operator+=(IntArray& ia);
  void operator+=(const int i) {insertNextValue(i);};
  // operator[] can be used on both left and right side of expression;
  // Note: if used on left hand side, user's responsibility to do range checking
  int& operator[](const int i) {return array[i];};
  void squeeze();
  int getSize();
  int getMaxId();
  void setMaxId(int id);
  int *getArray();
  void reset();

private:
  int *array;   // pointer to data
  int size;       // allocated size of data
  int max_id;     // maximum index inserted thus iar
  int extend;     // grow array by this point
  int *resize(const int sz);  // function to resize data
};

#endif

