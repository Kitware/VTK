#include <iostream.h>
#include "LinkList.hh"

vlLinkList::vlLinkList(const int sz, const int ext)
{
  this->Size = sz;
  this->Array = new vlLink[sz];
  this->Extend = ext;
  this->MaxId = -1;
}

vlLinkList::~vlLinkList()
{
  delete [] this->Array;
}

//
// Add a link to structure
//
void vlLinkList::InsertLink(const int id, const unsigned short ncells, int *cells)
{
  vlLink *link;

  if ( id >= this->Size ) this->Resize(id);
  if ( id > this->MaxId ) this->MaxId = id;

  link = this->Array + id;
  link->ncells = ncells;
  for (unsigned short i=0; i<ncells; i++) link->cells[i] = cells[i];

  return;
}

int vlLinkList::InsertNextLink(const unsigned short ncells, int *cells)
{
  this->InsertLink (++this->MaxId,ncells,cells);
  return this->MaxId;
}

void vlLinkList::Squeeze()
{
  this->Resize (this->MaxId+1);
}

void vlLinkList::Reset()
{
  this->MaxId = -1;
}
//
// Private function does "reallocate"
//
vlLink *vlLinkList::Resize(const int sz)
{
  int i;
  vlLink *newArray;
  int newSize;

  if ( sz >= this->Size ) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  if ( (newArray = new vlLink[newSize]) == 0 )
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
