//
//  Dynamic, self adjusting integer array
//
#ifndef __vlIntArray_h
#define __vlIntArray_h

#include "Object.h"

class vlIntArray : public vlObject 
{
public:
  vlIntArray():Array(0),Size(0),MaxId(-1),Extend(1000) {};
  int Initialize(const int sz, const int ext);
  vlIntArray(const int sz, const int ext);
  vlIntArray(const vlIntArray& ia);
  ~vlIntArray();
  int GetValue(const int id) {return this->Array[id];};
  int *GetPtr(const int id) {return this->Array + id;};
  vlIntArray &InsertValue(const int id, const int i);
  int InsertNextValue(const int i);
  vlIntArray &operator=(vlIntArray& ia);
  void operator+=(vlIntArray& ia);
  void operator+=(const int i) {this->InsertNextValue(i);};
  // operator[] can be used on both left and right side of expression;
  // Note: if used on left hand side, user's responsibility to do range checking
  int& operator[](const int i) {return this->Array[i];};
  void Squeeze();
  int GetSize();
  int GetMaxId();
  void SetMaxId(int id);
  int *GetArray();
  void Reset();

private:
  int *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus iar
  int Extend;     // grow array by this point
  int *Resize(const int sz);  // function to resize data
};

#endif

