//
//  Dynamic, self adjusting integer array
//
//  Assumptions:
//    - no bounds/range checking -> user responsibility
//    - the Register/Free methods called only by container class
//
#include "IntArray.h"

//
// External integer array provided; responsibility of user 
// to manage memory
//
IntArray::Initialize(const int sz, const int ext)
{
  if ( array != 0 ) delete [] array;

  size = ( sz > 0 ? sz : 1);
  if ( (array = new int[sz]) == 0 ) return 0;
  extend = ( ext > 0 ? ext : 1);
  max_id = -1;

  return 1;
}

IntArray::IntArray(const int sz, const int ext)
{
  size = ( sz > 0 ? sz : 1);
  array = new int[sz];
  extend = ( ext > 0 ? ext : 1);
  max_id = -1;
}

IntArray::~IntArray()
{
#ifdef DEBUG
  cout << "Destructor\n";
#endif

  delete [] array;
}

IntArray::IntArray(const IntArray& ia)
{
  int i;
#ifdef DEBUG
  cout << "Copy constructor\n";
#endif

  max_id = ia.max_id;
  size = ia.size;
  extend = ia.extend;

  array = new int[size];
  for (i=0; i<max_id; i++)
    array[i] = ia.array[i];

}

IntArray& IntArray::operator=(IntArray& ia)
{
  int i;

#ifdef DEBUG
  cout << "Assignment\n";
#endif

  if ( this != &ia )
  {
    delete [] array;

    max_id = ia.max_id;
    size = ia.size;
    extend = ia.extend;

    array = new int[size];
    for (i=0; i<=max_id; i++)
      array[i] = ia.array[i];
    
  }
  return *this;
}

//
// Copy on write if used by more than one object
//
void IntArray::operator+=(IntArray& ia)
{
  IntArray *instance=this;
  int i, sz;

#ifdef DEBUG
  cout << "Add method\n";
#endif

  if ( size <= (sz = max_id + ia.max_id + 2) ) resize(sz);

  for (i=0; i<=ia.max_id; i++)
  {
    instance->array[max_id+1+i] = ia.array[i];
  }
  max_id += ia.max_id + 1;

}

//
// Copy on write if used by more than one object
//
IntArray& IntArray::insertValue(const int id, const int i)
{
  IntArray *instance=this;

#ifdef DEBUG
  cout << "insert value\n";
#endif

  if ( id >= size ) resize(id);

  array[id] = i;
  if ( id > max_id ) max_id = id;

  return *this;
}

int IntArray::insertNextValue(const int i)
{
  insertValue (++max_id,i);
  return max_id;
}

void IntArray::squeeze()
{
  resize (max_id+1);
}

int IntArray::getSize()
{
  return size;    
}

int IntArray::getMaxId()
{
  return max_id;
}

void IntArray::setMaxId(int id)
{
  max_id = (id < size ? id : size-1);
}

int *IntArray::getArray()
{
  return array;
}

void IntArray::reset()
{
  max_id = -1;
}
//
// Private function does "reallocate"
//
int *IntArray::resize(const int sz)
{
  int i;
  int *new_array;
  int new_size;

#ifdef DEBUG
  cout << "resize\n";
#endif

  if ( sz >= size )   new_size = size + extend*(((sz-size)/extend)+1);
  else new_size = sz;

  if ( (new_array = new int[new_size]) == 0 )
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
