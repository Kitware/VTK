//
// Define link array
//
#ifndef LinkList_h
#define LinkList_h

#include "Params.h"

struct Link {
    unsigned short ncells;
    int *cells;
};

class LinkList {
public:
  LinkList():array(0),size(0),max_id(-1),extend(1000) {};
  LinkList(const int sz, const int ext);
  ~LinkList();
  Link &getLink(const int id) {return array[id];};
  unsigned short getNcells(const int id) {return array[id].ncells;};
  int *getCells(const int id) {return array[id].cells;};
  void insertLink(const int id, const unsigned short ncells, int *cells);
  int insertNextLink(const unsigned short ncells, int *cells);
  void squeeze();
  void reset();

private:
  Link *array;   // pointer to data
  int size;       // allocated size of data
  int max_id;     // maximum index inserted thus far
  int extend;     // grow array by this point
  Link *resize(const int sz);  // function to resize data
};


#endif
