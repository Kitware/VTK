//
//  Dynamic, self adjusting floating point array
//
//  Assumptions:
//    - no bounds/range checking -> user responsibility
//    - the Register/unRegister methods called only by container class
//
#include "FArray.h"

int vlFloatArray::Initialize(const int sz, const int ext)
{
  if ( this->Array ) delete [] this->Array;

  this->Size = ( sz > 0 ? sz : 1);
  if ( (this->Array = new float[sz]) == 0 ) return 0;
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;

  return 1;
}

vlFloatArray::vlFloatArray(const int sz, const int ext)
{
  this->Size = ( sz > 0 ? sz : 1);
  this->Array = new float[sz];
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;
}

vlFloatArray::~vlFloatArray()
{
  if ( this->Debug ) cerr << "Destructor\n";

  delete [] this->Array;
}

vlFloatArray::vlFloatArray(const vlFloatArray& fa)
{
  int i;
  if ( this->Debug ) cerr << "Copy constructor\n";

  this->MaxId = fa.MaxId;
  this->Size = fa.Size;
  this->Extend = fa.Extend;

  this->Array = new float[this->Size];
  for (i=0; i<this->MaxId; i++)
    this->Array[i] = fa.Array[i];

}

vlFloatArray& vlFloatArray::operator=(const vlFloatArray& fa)
{
  int i;

  if ( this->Debug ) cerr << "Assignment\n";

  if ( this != &fa )
    {
    delete [] this->Array;

    this->MaxId = fa.MaxId;
    this->Size = fa.Size;
    this->Extend = fa.Extend;

    this->Array = new float[this->Size];
    for (i=0; i<=this->MaxId; i++)
      this->Array[i] = fa.Array[i];
    }
  return *this;
}

//
// Copy on write if used by more than one object
//
vlFloatArray& vlFloatArray::operator+=(const vlFloatArray& fa)
{
  int i, sz;

  if ( this->Debug ) cerr << "Add method\n";

  if ( this->Size <= (sz = this->MaxId + fa.MaxId + 2) ) this->Resize(sz);

  for (i=0; i<=fa.MaxId; i++)
    {
    this->Array[this->MaxId+1+i] = fa.Array[i];
    }
  this->MaxId += fa.MaxId + 1;

  return *this;
}

//
// Copy on write if used by more than one object
//
vlFloatArray& vlFloatArray::InsertValue(const int id, const float f)
{
  if ( this->Debug ) cerr << "insert value\n";

  if ( id >= this->Size ) this->Resize(id);

  this->Array[id] = f;
  if ( id > this->MaxId ) this->MaxId = id;

  return *this;
}

int vlFloatArray::InsertNextValue(const float f)
{
  this->InsertValue (++this->MaxId,f);
  return this->MaxId;
}

void vlFloatArray::Squeeze()
{
  this->Resize (this->MaxId+1);
}

int vlFloatArray::GetSize()
{
  return this->Size;    
}

int vlFloatArray::GetMaxId()
{
  return this->MaxId;
}

void vlFloatArray::SetMaxId(int id)
{
  this->MaxId = (id < this->Size ? id : this->Size-1);
}

float *vlFloatArray::GetArray()
{
  return this->Array;
}

void vlFloatArray::Reset()
{
  this->MaxId = -1;
}
//
// Private function does "reallocate"
//
float *vlFloatArray::Resize(const int sz)
{
  int i;
  float *newArray;
  int newSize;

  if ( this->Debug ) cerr << "Resize\n";

  if (sz >= this->Size) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  if ( (newArray = new float[newSize]) == 0 )
    { 
    cerr << "Cannot allocate memory\n";
    return 0;
    }

  for (i=0; i<sz && i<this->Size; i++)
      newArray[i] = this->Array[i];

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}
