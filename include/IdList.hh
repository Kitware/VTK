//
// List of id's.  Used for passing information back and forth between objects.
//
#ifndef vlIdList_h
#define vlIdList_h

#include "IntArray.hh"

class vlIdList {
public:
  vlIdList(const int sz, const int ext=100):Ia(sz,ext) {};
  ~vlIdList() {};
  int &operator[](const int id) {return this->Ia[id];};
  void Reset() {this->Ia.Reset();};
  int NumberOfIds() {return (this->Ia.GetMaxId() + 1);};
  int InsertNextId(const int i) {return Ia.InsertNextValue(i);};
  void operator+=(vlIdList& ids) {this->Ia += ids.Ia;};
  void operator+=(const int i) {this->Ia += i;};
  int getChunk(const int sz) { // get chunk of memory
    int pos = this->Ia.GetMaxId()+1;
    this->Ia.InsertValue(pos+sz-1,0);
    return pos;
  }
private:
  vlIntArray Ia;
};

#endif
