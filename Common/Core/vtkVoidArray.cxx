/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoidArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVoidArray.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkVoidArray);

typedef void *voidPtr;

// Instantiate object.
vtkVoidArray::vtkVoidArray()
  : NumberOfPointers(0),Size(0),Array(NULL)
{
}

vtkVoidArray::~vtkVoidArray()
{
  delete [] this->Array;
}

// Allocate memory for this array. Delete old storage only if necessary.
int vtkVoidArray::Allocate(vtkIdType sz, vtkIdType vtkNotUsed(ext))
{
  if ( sz > this->Size || this->Array != NULL )
  {
    delete [] this->Array;

    this->Size = ( sz > 0 ? sz : 1);
    if ( (this->Array = new voidPtr[this->Size]) == NULL )
    {
      return 0;
    }
  }

  this->NumberOfPointers = 0;

  return 1;
}

// Release storage and reset array to initial state.
void vtkVoidArray::Initialize()
{
  delete [] this->Array;
  this->Array = NULL;
  this->Size = 0;
  this->NumberOfPointers = 0;
}

// Deep copy of another void array.
void vtkVoidArray::DeepCopy(vtkVoidArray *va)
{
  // Do nothing on a NULL input.
  if (va == NULL)
  {
    return;
  }

  if ( this != va )
  {
    delete [] this->Array;

    this->NumberOfPointers = va->NumberOfPointers;
    this->Size = va->Size;

    this->Array = new voidPtr[this->Size];
    memcpy(this->Array, va->GetVoidPointer(0), this->Size*sizeof(void *));
  }
}

void** vtkVoidArray::WritePointer(vtkIdType id,
                                  vtkIdType number)
{
  vtkIdType newSize=id+number;
  if ( newSize > this->Size )
  {
    this->ResizeAndExtend(newSize);
  }
  if ( newSize > this->NumberOfPointers )
  {
    this->NumberOfPointers = newSize;
  }
  return this->Array + id;
}

void vtkVoidArray::InsertVoidPointer(vtkIdType id, void* p)
{
  if ( id >= this->Size )
  {
    if (!this->ResizeAndExtend(id+1))
    {
      return;
    }
  }
  this->Array[id] = p;
  if ( id >= this->NumberOfPointers )
  {
    this->NumberOfPointers = id+1;
  }
}

vtkIdType vtkVoidArray::InsertNextVoidPointer(void* p)
{
  this->InsertVoidPointer(this->NumberOfPointers,p);
  return this->NumberOfPointers-1;
}

// Protected function does "reallocate"
//
void** vtkVoidArray::ResizeAndExtend(vtkIdType sz)
{
  void** newArray;
  vtkIdType newSize;

  if ( sz > this->Size )
  {
    newSize = this->Size + sz;
  }
  else if (sz == this->Size)
  {
    return this->Array;
  }
  else
  {
    newSize = sz;
  }

  if (newSize <= 0)
  {
    this->Initialize();
    return 0;
  }

  if ( (newArray = new voidPtr[newSize]) == NULL )
  {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
  }

  memcpy(newArray, this->Array,
         (sz < this->Size ? sz : this->Size) * sizeof(voidPtr));

  if (newSize < this->Size)
  {
    this->NumberOfPointers = newSize;
  }
  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

void vtkVoidArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->Array)
  {
    os << indent << "Array: " << this->Array << "\n";
  }
  else
  {
    os << indent << "Array: (null)\n";
  }
}
