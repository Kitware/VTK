//
//  Dynamic, self adjusting floating point array
//
//  Assumptions:
//    - no bounds/range checking -> user responsibility
//    - the Register/unRegister methods called only by container class
//
#include "FArray.h"

int FloatArray::Initialize(const int sz, const int ext)
{
  if ( array != 0 ) delete [] array;

  size = ( sz > 0 ? sz : 1);
  if ( (array = new float[sz]) == 0 ) return 0;
  extend = ( ext > 0 ? ext : 1);
  max_id = -1;

  return 1;
}

FloatArray::FloatArray(const int sz, const int ext)
{
  size = ( sz > 0 ? sz : 1);
  array = new float[sz];
  extend = ( ext > 0 ? ext : 1);
  max_id = -1;
}

FloatArray::~FloatArray()
{
#ifdef DEBUG
  cout << "Destructor\n";
#endif

  delete [] array;
}

FloatArray::FloatArray(const FloatArray& fa)
{
  int i;
#ifdef DEBUG
  cout << "Copy constructor\n";
#endif

  max_id = fa.max_id;
  size = fa.size;
  extend = fa.extend;

  array = new float[size];
  for (i=0; i<max_id; i++)
    array[i] = fa.array[i];

}

FloatArray& FloatArray::operator=(const FloatArray& fa)
{
  int i;

#ifdef DEBUG
  cout << "Assignment\n";
#endif

  if ( this != &fa )
  {
    delete [] array;

    max_id = fa.max_id;
    size = fa.size;
    extend = fa.extend;

    array = new float[size];
    for (i=0; i<=max_id; i++)
      array[i] = fa.array[i];
    
  }
  return *this;
}

//
// Copy on write if used by more than one object
//
FloatArray& FloatArray::operator+=(const FloatArray& fa)
{
  int i, sz;

#ifdef DEBUG
  cout << "Add method\n";
#endif

  if ( size <= (sz = max_id + fa.max_id + 2) ) resize(sz);

  for (i=0; i<=fa.max_id; i++)
  {
    array[max_id+1+i] = fa.array[i];
  }
  max_id += fa.max_id + 1;

  return *this;
}

//
// Copy on write if used by more than one object
//
FloatArray& FloatArray::insertValue(const int id, const float f)
{
  FloatArray *instance=this;

#ifdef DEBUG
  cout << "insert value\n";
#endif

  if ( id >= size ) resize(id);

  array[id] = f;
  if ( id > max_id ) max_id = id;

  return *this;
}

int FloatArray::insertNextValue(const float f)
{
  insertValue (++max_id,f);
  return max_id;
}

void FloatArray::squeeze()
{
  resize (max_id+1);
}

int FloatArray::getSize()
{
  return size;    
}

int FloatArray::getMaxId()
{
  return max_id;
}

void FloatArray::setMaxId(int id)
{
  max_id = (id < size ? id : size-1);
}

float *FloatArray::getArray()
{
  return array;
}

void FloatArray::reset()
{
  max_id = -1;
}
//
// Private function does "reallocate"
//
float *FloatArray::resize(const int sz)
{
  int i;
  float *new_array;
  int new_size;

#ifdef DEBUG
  cout << "resize\n";
#endif

  if ( sz >= size )   new_size = size + extend*(((sz-size)/extend)+1);
  else new_size = sz;

  if ( (new_array = new float[new_size]) == 0 )
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
