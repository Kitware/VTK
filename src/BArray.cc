/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BArray.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include "BArray.hh"


// Description:
// Get the data at a particular index.
int vtkBitArray::GetValue(const int id) 
{
  if (this->Array[id/8]&(0x80 >> (id%8))) return 1; return 0;
};

// Description:
// Allocate memory for this array. Delete old storage if present.
int vtkBitArray::Allocate(const int sz, const int ext)
{
  if ( this->Array != NULL ) delete [] this->Array;

  this->Size = ( (sz/8) > 0 ? sz : 1);
  if ( (this->Array = new unsigned char[(this->Size+7)/8]) == NULL ) return 0;
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;

  return 1;
}

// Description:
// Release storage and reset array to initial state.
void vtkBitArray::Initialize()
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
// Construct with specified storage and extend value.
vtkBitArray::vtkBitArray(const int sz, const int ext)
{
  this->Size = ( (sz/8) > 0 ? sz : 1);
  this->Array = new unsigned char[(this->Size+7)/8];
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;
}

vtkBitArray::~vtkBitArray()
{
  delete [] this->Array;
}

// Description:
// Construct array from another array. Copy each element of other array.
vtkBitArray::vtkBitArray(const vtkBitArray& ia)
{
  int i;

  this->MaxId = ia.MaxId;
  this->Size = ia.Size;
  this->Extend = ia.Extend;

  this->Array = new unsigned char[(this->Size+7)/8];
  for (i=0; i<this->MaxId; i++)
    this->Array[i] = ia.Array[i];

}

// Description:
// Deep copy of another array.
vtkBitArray& vtkBitArray::operator=(const vtkBitArray& ia)
{
  int i;

  if ( this != &ia )
    {
    delete [] this->Array;

    this->MaxId = ia.MaxId;
    this->Size = ia.Size;
    this->Extend = ia.Extend;

    this->Array = new unsigned char[(this->Size+7)/8];
    for (i=0; i<=this->MaxId; i++)
      this->Array[i] = ia.Array[i];
    }
  return *this;
}

// Description:
// Append one array onto the end of this array.
vtkBitArray& vtkBitArray::operator+=(const vtkBitArray& ia)
{
  int i, sz;

  if ( this->Size <= (sz = this->MaxId + ia.MaxId + 2) ) this->Resize(sz);

  for (i=0; i<=ia.MaxId; i++)
    {
    this->Array[this->MaxId+1+i] = ia.Array[i];
    }
  this->MaxId += ia.MaxId + 1;
  return *this;
}

void vtkBitArray::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Array: " << this->Array << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "MaxId: " << this->MaxId << "\n";
  os << indent << "Extend size: " << this->Extend << "\n";
}

//
// Private function does "reallocate". Sz is the number of "bits", and we
// can allocate only 8-bit bytes.
unsigned char *vtkBitArray::Resize(const int sz)
{
  unsigned char *newArray;
  int newSize;

  if ( sz >= this->Size ) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  if ( (newArray = new unsigned char[(newSize+7)/8]) == NULL )
    {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  memcpy(newArray, this->Array, 
         (sz < this->Size ? sz : this->Size) * sizeof(char));

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}
