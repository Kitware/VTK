/*=========================================================================

  Program:   Visualization Library
  Module:    SArray.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SArray.hh"

// Description:
// Allocate memory for this array. Delete old storage if present.
int vlShortArray::Allocate(const int sz, const int ext)
{
  if ( this->Array != NULL ) delete [] this->Array;

  this->Size = ( sz > 0 ? sz : 1);
  if ( (this->Array = new short[sz]) == NULL ) return 0;
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;

  return 1;
}

// Description:
// Release storage and reset array to initial state.
void vlShortArray::Initialize()
{
  if ( this->Array != NULL )
    {
    delete [] this->Array;
    this->Array = NULL;
    }
  this->Size = 0;
  this->MaxId = -1;
}

// Description:
// Construct with specified storage size and extend value.
vlShortArray::vlShortArray(const int sz, const int ext)
{
  this->Size = ( sz > 0 ? sz : 1);
  this->Array = new short[sz];
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;
}

vlShortArray::~vlShortArray()
{
  delete [] this->Array;
}

// Description:
// Construct array from another array. Copy each element of other array.
vlShortArray::vlShortArray(const vlShortArray& sa)
{
  int i;

  this->MaxId = sa.MaxId;
  this->Size = sa.Size;
  this->Extend = sa.Extend;

  this->Array = new short[this->Size];
  for (i=0; i<this->MaxId; i++)
    this->Array[i] = sa.Array[i];

}

// Description:
// Deep copy of another array.
vlShortArray& vlShortArray::operator=(const vlShortArray& sa)
{
  int i;

  if ( this != &sa )
    {
    delete [] this->Array;

    this->MaxId = sa.MaxId;
    this->Size = sa.Size;
    this->Extend = sa.Extend;

    this->Array = new short[this->Size];
    for (i=0; i<=this->MaxId; i++)
      this->Array[i] = sa.Array[i];
    }
  return *this;
}

// Description:
// Append one array onto the end of this array.
void vlShortArray::operator+=(const vlShortArray& sa)
{
  int i, sz;

  if ( this->Size <= (sz = this->MaxId + sa.MaxId + 2) ) this->Resize(sz);

  for (i=0; i<=sa.MaxId; i++)
    {
    this->Array[this->MaxId+1+i] = sa.Array[i];
    }
  this->MaxId += sa.MaxId + 1;
}

void vlShortArray::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlShortArray::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);

    os << indent << "Array: " << this->Array << "\n";
    os << indent << "Size: " << this->Size << "\n";
    os << indent << "MaxId: " << this->MaxId << "\n";
    os << indent << "Extend size: " << this->Extend << "\n";
    }
}

//
// Private function does "reallocate"
//
short *vlShortArray::Resize(const int sz)
{
  int i;
  short *newArray;
  int newSize;

  if ( sz >= this->Size ) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  if ( (newArray = new short[newSize]) == NULL )
    {
    vlErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  for (i=0; i<sz && i<this->Size; i++)
      newArray[i] = this->Array[i];

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}
