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


LinkList::LinkList(const int sz, const int ext)
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

  size = sz;
  array = new Link[sz];
  extend = ext;
  max_id = -1;
}

LinkList::~LinkList()
{
#ifdef DEBUG
  cout << "Destructor\n";
#endif

  delete [] array;
}

//
// Add a link to structure
//
void LinkList::insertLink(const int id, const unsigned short ncells, int *cells)
{
#ifdef DEBUG
  cout << "insert value\n";
#endif

  Link *link;

  if ( id >= size ) resize(id);
  if ( id > max_id ) max_id = id;

  link = array + id;
  link->ncells = ncells;
  for (unsigned short i=0; i<ncells; i++) link->cells[i] = cells[i];

  return;
}

int LinkList::insertNextLink(const unsigned short ncells, int *cells)
{
  insertLink (++max_id,ncells,cells);
  return max_id;
}

void LinkList::squeeze()
{
  resize (max_id+1);
}

void LinkList::reset()
{
  max_id = -1;
}
//
// Private function does "reallocate"
//
Link *LinkList::resize(const int sz)
{
  int i;
  Link *new_array;
  int new_size;

#ifdef DEBUG
  cout << "resize\n";
#endif

  if ( sz >= size )   new_size = size + extend*(((sz-size)/extend)+1);
  else new_size = sz;

  if ( (new_array = new Link[new_size]) == 0 )
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
