/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FArray.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "FArray.hh"

// Description:
// Allocate memory for this array. Delete old storage if present.
int vtkFloatArray::Allocate(const int sz, const int ext)
{
  if ( this->Array ) delete [] this->Array;

  this->Size = ( sz > 0 ? sz : 1);
  if ( (this->Array = new float[this->Size]) == NULL ) return 0;
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;

  return 1;
}

// Description:
// Release storage and reset array to initial state.
void vtkFloatArray::Initialize()
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
vtkFloatArray::vtkFloatArray(const int sz, const int ext)
{
  this->Size = ( sz > 0 ? sz : 1);
  this->Array = new float[this->Size];
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;
}

vtkFloatArray::~vtkFloatArray()
{
  if (this->Array)
    {
    delete [] this->Array;
    }
}

// Description:
// Construct array from another array. Copy each element of other array.
vtkFloatArray::vtkFloatArray(const vtkFloatArray& fa)
{
  int i;

  this->MaxId = fa.MaxId;
  this->Size = fa.Size;
  this->Extend = fa.Extend;

  this->Array = new float[this->Size];
  for (i=0; i<this->MaxId; i++)
    this->Array[i] = fa.Array[i];

}

// Description:
// Deep copy of another array.
vtkFloatArray& vtkFloatArray::operator=(const vtkFloatArray& fa)
{
  int i;

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

// Description:
// Append one array onto the end of this array.
void vtkFloatArray::operator+=(const vtkFloatArray& fa)
{
  int i, sz;

  if ( this->Size <= (sz = this->MaxId + fa.MaxId + 2) ) this->Resize(sz);

  for (i=0; i<=fa.MaxId; i++)
    {
    this->Array[this->MaxId+1+i] = fa.Array[i];
    }
  this->MaxId += fa.MaxId + 1;
}

void vtkFloatArray::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Array: " << this->Array << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "MaxId: " << this->MaxId << "\n";
  os << indent << "Extend size: " << this->Extend << "\n";
}

// Protected function does "reallocate"
//
float *vtkFloatArray::Resize(const int sz)
{
  float *newArray;
  int newSize;

  if (sz >= this->Size) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  if ( (newArray = new float[newSize]) == NULL )
    { 
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }


  if (this->Array)
    {
    memcpy(newArray, this->Array,
	   (sz < this->Size ? sz : this->Size) * sizeof(float));
    delete [] this->Array;
    }

  this->Size = newSize;
  this->Array = newArray;

  return this->Array;
}
