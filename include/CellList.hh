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


CellList::CellList(const int sz, const int ext)
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

  size = sz;
  array = new Cell[sz];
  extend = ext;
  max_id = -1;
}

CellList::~CellList()
{
#ifdef DEBUG
  cout << "Destructor\n";
#endif

  delete [] array;
}

//
// Add a cell to structure
//
void CellList::insertCell(const int id, const short type, const int loc)
{
#ifdef DEBUG
  cout << "insert value\n";
#endif

  Cell *cell;

  if ( id >= size ) resize(id);
  if ( id > max_id ) max_id = id;

  cell = array + id;
  cell->type = type;
  cell->loc = loc;

  return;
}

int CellList::insertNextCell(const short type, const int loc)
{
  insertCell (++max_id,type,loc);
  return max_id;
}

void CellList::squeeze()
{
  resize (max_id+1);
}

void CellList::reset()
{
  max_id = -1;
}
//
// Private function does "reallocate"
//
Cell *CellList::resize(const int sz)
{
  int i;
  Cell *new_array;
  int new_size;

#ifdef DEBUG
  cout << "resize\n";
#endif

  if ( sz >= size )   new_size = size + extend*(((sz-size)/extend)+1);
  else new_size = sz;

  if ( (new_array = new Cell[new_size]) == 0 )
  {
    cout << "Cannot allocate memory\n";
    return 0;
  }

  for (i=0; i<sz && i<size; i++)
      new_array[i] = array[i];

  size = new_size;
  delete [] array;
  array = new_array;

  return array;
}

#endif
