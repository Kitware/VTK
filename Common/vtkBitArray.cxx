/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBitArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBitArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkBitArray, "1.59");
vtkStandardNewMacro(vtkBitArray);

// Instantiate object.
vtkBitArray::vtkBitArray(vtkIdType numComp)
{
  this->NumberOfComponents = (numComp < 1 ? 1 : numComp);
  this->Array = NULL;
  this->TupleSize = 3;
  this->Tuple = new double[this->TupleSize]; //used for conversion
  this->SaveUserArray = 0;
}

vtkBitArray::~vtkBitArray()
{
  if ((this->Array) && (!this->SaveUserArray))
    {
    delete [] this->Array;
    }
  delete [] this->Tuple;
}

unsigned char *vtkBitArray::WritePointer(vtkIdType id, vtkIdType number)
{
  vtkIdType newSize=id+number;
  if ( newSize > this->Size )
    {
    this->ResizeAndExtend(newSize);
    }
  if ( (--newSize) > this->MaxId )
    {
    this->MaxId = newSize;
    }
  return this->Array + id/8;
}

// This method lets the user specify data to be held by the array.  The 
// array argument is a pointer to the data.  size is the size of 
// the array supplied by the user.  Set save to 1 to keep the class
// from deleting the array when it cleans up or reallocates memory.
// The class uses the actual array provided; it does not copy the data 
// from the suppled array.
void vtkBitArray::SetArray(unsigned char* array, vtkIdType size, int save)
{
  
  if ((this->Array) && (!this->SaveUserArray))
    {
      vtkDebugMacro (<< "Deleting the array...");
      delete [] this->Array;
    }
  else 
    {
      vtkDebugMacro (<<"Warning, array not deleted, but will point to new array.");
    }

  vtkDebugMacro(<<"Setting array to: " << array);

  this->Array = array;
  this->Size = size;
  this->MaxId = size-1;
  this->SaveUserArray = save;
}

// Get the data at a particular index.
int vtkBitArray::GetValue(vtkIdType id)
{
  if (this->Array[id/8]&(0x80 >> (id%8)))
    {
    return 1;
    }
  return 0;
}

// Allocate memory for this array. Delete old storage only if necessary.
int vtkBitArray::Allocate(vtkIdType sz, vtkIdType vtkNotUsed(ext))
{
  if ( sz > this->Size )
    {
    if (( this->Array != NULL ) && (!this->SaveUserArray))
      {
      delete [] this->Array;
      }
    this->Size = ( sz > 0 ? sz : 1);
    if ( (this->Array = new unsigned char[(this->Size+7)/8]) == NULL )
      {
      return 0;
      }
    this->SaveUserArray = 0;
    }

  this->MaxId = -1;

  return 1;
}

// Release storage and reset array to initial state.
void vtkBitArray::Initialize()
{
  if (( this->Array != NULL ) && (!this->SaveUserArray))
    {
    delete [] this->Array;
    }
  this->Array = NULL;
  this->Size = 0;
  this->MaxId = -1;
  this->SaveUserArray = 0;
}

// Deep copy of another bit array.
void vtkBitArray::DeepCopy(vtkDataArray *ia)
{
  // Do nothing on a NULL input.
  if (ia == NULL)
    {
    return;
    }

  if (ia->GetDataType() != VTK_BIT)
    {
    vtkIdType numTuples = ia->GetNumberOfTuples();
    this->NumberOfComponents = ia->GetNumberOfComponents();
    this->SetNumberOfTuples(numTuples);

    for (vtkIdType i = 0; i < numTuples; i++)
      {
      this->SetTuple(i, ia->GetTuple(i));
      }
    return;
    }

  if ( this != ia )
    {
    if ((this->Array) && (!this->SaveUserArray))
      {
      delete [] this->Array;
      }

    this->NumberOfComponents = ia->GetNumberOfComponents();
    this->MaxId = ia->GetMaxId();
    this->Size = ia->GetSize();
    this->SaveUserArray = 0;

    this->Array = new unsigned char[(this->Size+7)/8];
    memcpy(this->Array, (unsigned char*)ia->GetVoidPointer(0),
           ((this->Size+7)/8)*sizeof(unsigned char));
    }
}

void vtkBitArray::PrintSelf(ostream& os, vtkIndent indent)
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

//
// Private function does "reallocate". Sz is the number of "bits", and we
// can allocate only 8-bit bytes.
unsigned char *vtkBitArray::ResizeAndExtend(vtkIdType sz)
{
  unsigned char *newArray;
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

  if ( (newArray = new unsigned char[(newSize+7)/8]) == NULL )
    {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  if (this->Array)
    {
    int usedSize = (sz < this->Size) ? sz : this->Size;

    memcpy(newArray, this->Array, 
         ((usedSize+7)/8)*sizeof(unsigned char));
    if (!this->SaveUserArray)
      {
        delete[] this->Array;
      }
    }

  if (newSize < this->Size)
    {
    this->MaxId = newSize-1;
    }
  this->Size = newSize;
  this->Array = newArray;
  this->SaveUserArray = 0;

  return this->Array;
}

void vtkBitArray::Resize(vtkIdType sz)
{
  unsigned char *newArray;
  vtkIdType newSize = sz*this->NumberOfComponents;

  if (newSize == this->Size)
    {
    return;
    }

  if (newSize <= 0)
    {
    this->Initialize();
    return;
    }

  if ( (newArray = new unsigned char[(newSize+7)/8]) == NULL )
    {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return;
    }

  if (this->Array)
    {
    int usedSize = (newSize < this->Size) ? newSize : this->Size;

    memcpy(newArray, this->Array, 
         ((usedSize+7)/8)*sizeof(unsigned char));
    if (!this->SaveUserArray)
      {
        delete[] this->Array;
      }
    }

  if (newSize < this->Size)
    {
    this->MaxId = newSize-1;
    }
  this->Size = newSize;
  this->Array = newArray;
  this->SaveUserArray = 0;

  return;
}

// Set the number of n-tuples in the array.
void vtkBitArray::SetNumberOfTuples(vtkIdType number)
{
  this->SetNumberOfValues(number*this->NumberOfComponents);
}

// Get a pointer to a tuple at the ith location. This is a dangerous method
// (it is not thread safe since a pointer is returned).
double *vtkBitArray::GetTuple(vtkIdType i)
{
  if ( this->TupleSize < this->NumberOfComponents )
    {
    this->TupleSize = this->NumberOfComponents;
    delete [] this->Tuple;
    this->Tuple = new double[this->TupleSize];
    }

  int loc = this->NumberOfComponents*i;
  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->Tuple[j] = (double)this->GetValue(loc+j);
    }

  return this->Tuple;
}

// Copy the tuple value into a user-provided array.
void vtkBitArray::GetTuple(vtkIdType i, double * tuple)
{
  vtkIdType loc = this->NumberOfComponents*i;

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    tuple[j] = (double)this->GetValue(loc+j);
    }
}

// Set the tuple value at the ith location in the array.
void vtkBitArray::SetTuple(vtkIdType i, const float * tuple)
{
  vtkIdType loc = i * this->NumberOfComponents; 

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->SetValue(loc+j,(int)tuple[j]);
    }
}

void vtkBitArray::SetTuple(vtkIdType i, const double * tuple)
{
  vtkIdType loc = i * this->NumberOfComponents; 

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->SetValue(loc+j,(int)tuple[j]);
    }
}

// Insert (memory allocation performed) the tuple into the ith location
// in the array.
void vtkBitArray::InsertTuple(vtkIdType i, const float * tuple)
{
  vtkIdType loc = this->NumberOfComponents*i;

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->InsertValue(loc+j,(int)tuple[j]);
    }
}

void vtkBitArray::InsertTuple(vtkIdType i, const double * tuple)
{
  vtkIdType loc = this->NumberOfComponents*i;

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->InsertValue(loc+j,(int)tuple[j]);
    }
}

// Insert (memory allocation performed) the tuple onto the end of the array.
vtkIdType vtkBitArray::InsertNextTuple(const float * tuple)
{
  for (int i=0; i<this->NumberOfComponents; i++)
    {
    this->InsertNextValue((int)tuple[i]);
    }

  return this->MaxId / this->NumberOfComponents;
}

vtkIdType vtkBitArray::InsertNextTuple(const double * tuple)
{
  for (int i=0; i<this->NumberOfComponents; i++)
    {
    this->InsertNextValue((int)tuple[i]);
    }

  return this->MaxId / this->NumberOfComponents;
}


void vtkBitArray::InsertComponent(vtkIdType i, int j, double c)
{
  this->InsertValue(i*this->NumberOfComponents + j, 
                    static_cast<int>(c));
}

// Set the data component at the ith tuple and jth component location.
// Note that i<NumberOfTuples and j<NumberOfComponents. Make sure enough
// memory has been allocated (use SetNumberOfTuples() and 
// SetNumberOfComponents()).
void vtkBitArray::SetComponent(vtkIdType i, int j, double c)
{
  this->SetValue(i*this->NumberOfComponents + j, static_cast<int>(c));
}
