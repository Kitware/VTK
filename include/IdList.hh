//
// List of id's.  Used for passing information back and forth between objects.
//
#ifndef IdList_h
#define IdList_h

#include "Params.h"
#include "IntArray.h"

class IdList {
public:
  IdList(const int sz, const int ext=100):ia(sz,ext) {};
  ~IdList() {};
  int &operator[](const int id) {return ia[id];};
  void reset() {ia.reset();};
  int numIds() {return (ia.getMaxId() + 1);};
  int insertNextId(const int i) {return ia.insertNextValue(i);};
  void operator+=(IdList& ids) {ia += ids.ia;};
  void operator+=(const int i) {ia += i;};
  int getChunk(const int sz) { // get chunk of memory
    int pos = ia.getMaxId()+1;
    ia.insertValue(pos+sz-1,0);
    return pos;
  }
private:
  IntArray ia;
};

#endif
