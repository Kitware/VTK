//
// Define cell array
//
#ifndef CellList_h
#define CellList_h

#include "Params.h"

struct Cell {
    short type;
    int loc;
};

class CellList {
public:
  CellList() : array(0),size(0),max_id(-1),extend(1000) {};
  CellList(const int sz, const int ext);
  ~CellList();
  Cell &getCell(const int id) {return array[id];};
  short getCellType(const int id) {return array[id].type;};
  int getCellLoc(const int id) {return array[id].loc;};
  void insertCell(const int id, const short type, const int loc);
  int insertNextCell(const short type, const int pos);
  void squeeze();
  void reset();

private:
  Cell *array;   // pointer to data
  int size;       // allocated size of data
  int max_id;     // maximum index inserted thus far
  int extend;     // grow array by this point
  Cell *resize(const int sz);  // function to resize data
};

#endif
