//
//  Dynamic, self adjusting floating point array
//
#ifndef __vlFloatArray_h
#define __vlFloatArray_h

#include "Object.hh"

class vlFloatArray : public vlObject 
{
public:
  vlFloatArray():Array(0),Size(0),MaxId(-1),Extend(1000) {};
  int Initialize(const int sz, const int ext=1000);
  vlFloatArray(const int sz, const int ext=1000);
  vlFloatArray(const vlFloatArray& fa);
  ~vlFloatArray();
  float GetValue(const int id) {return this->Array[id];};
  float *GetPtr(const int id) {return this->Array + id;};
  vlFloatArray &InsertValue(const int id, const float f);
  int InsertNextValue(const float f);
  vlFloatArray &operator=(const vlFloatArray& fa);
  vlFloatArray &operator+=(const vlFloatArray& fa);
  void operator+=(const float f) {this->InsertNextValue(f);};
  // operator[] can be used on both left and right side of expression;
  // Note: if used on left hand side, user's responsibility to do range checking
  float& operator[](const int i) {return this->Array[i];};
  void Squeeze();
  int GetSize();
  int GetMaxId();
  void SetMaxId(int id);
  float *GetArray();
  void Reset();
  virtual char *GetClassName() {return "vlFloatArray";};

private:
  float *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  float *Resize(const int sz);  // function to resize data
};

#endif
