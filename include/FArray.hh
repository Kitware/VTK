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
  vlFloatArray &InsertValue(const int id, const float f)
    {if ( id >= this->Size ) this->Resize(id);
     this->Array[id] = f;
     if ( id > this->MaxId ) this->MaxId = id;
     return *this;
    }
  int InsertNextValue(const float f)
    {this->InsertValue (++this->MaxId,f); return this->MaxId;};
  vlFloatArray &operator=(const vlFloatArray& fa);
  vlFloatArray &operator+=(const vlFloatArray& fa);
  void operator+=(const float f) {this->InsertNextValue(f);};
  // operator[] can be used on both left and right side of expression;
  // Note: if used on lh side, user's responsibility to do range checking
  float& operator[](const int i) {return this->Array[i];};
  void Squeeze() {this->Resize (this->MaxId+1);};
  int GetSize() {return this->Size;};
  int GetMaxId() {return this->MaxId;};
  void SetMaxId(int id) {this->MaxId = (id < this->Size ? id : this->Size-1);};
  float *GetArray() {return this->Array;};
  void Reset() {this->MaxId = -1;};
  virtual char *GetClassName() {return "vlFloatArray";};
  void PrintSelf(ostream& os);

private:
  float *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  float *Resize(const int sz);  // function to resize data
};

#endif
