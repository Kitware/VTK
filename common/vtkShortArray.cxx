/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShortArray.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkShortArray.h"

// Description:
// Instantiate object.
vtkShortArray::vtkShortArray(int numComp)
{
  this->NumberOfComponents = (numComp < 1 ? 1 : numComp);
  this->Array = NULL;
  this->TupleSize = 3;
  this->Tuple = new float[this->TupleSize]; //used for conversion
}

vtkShortArray::~vtkShortArray()
{
  if (this->Array) delete [] this->Array;
  delete [] this->Tuple;
}

// Description:
// Allocate memory for this array. Delete old storage only if necessary.
int vtkShortArray::Allocate(const int sz, const int ext)
{
  if ( sz > this->Size || this->Array == NULL )
    {
    delete [] this->Array;

    this->Size = ( sz > 0 ? sz : 1);
    if ( (this->Array = new short[this->Size]) == NULL ) return 0;
    }

  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;

  return 1;
}

// Description:
// Release storage and reset array to initial state.
void vtkShortArray::Initialize()
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
// Deep copy of another short array.
void vtkShortArray::DeepCopy(vtkDataArray& sa)
{
  if ( sa.GetDataType() != VTK_SHORT )
    {
    vtkDataArray::DeepCopy(sa);
    return;
    }
  
  if ( this != &sa )
    {
    if (this->Array) delete [] this->Array;

    this->NumberOfComponents = sa.GetNumberOfComponents();
    this->MaxId = sa.GetMaxId();
    this->Size = sa.GetSize();
    this->Extend = sa.GetExtend();

    this->Array = new short[this->Size];
    memcpy(this->Array, (short *)sa.GetVoidPointer(0), this->Size*sizeof(short));
    }
}

void vtkShortArray::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Array: " << this->Array << "\n";
}

//
// Private function does "reallocate"
//
short *vtkShortArray::Resize(const int sz)
{
  short *newArray;
  int newSize;

  if ( sz > this->Size ) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else if (sz == this->Size)
    return this->Array;
  else newSize = sz;

  if ( (newArray = new short[newSize]) == NULL )
    {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  memcpy(newArray, this->Array, 
         (sz < this->Size ? sz : this->Size) * sizeof(short));

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

// Description:
// Set the number of n-tuples in the array.
void vtkShortArray::SetNumberOfTuples(const int number)
{
  this->SetNumberOfValues(number*this->NumberOfComponents);
}

// Description:
// Get a pointer to a tuple at the ith location. This is a dangerous method
// (it is not thread safe since a pointer is returned).
float *vtkShortArray::GetTuple(const int i) 
{
  if ( this->TupleSize < this->NumberOfComponents )
    {
    this->TupleSize = this->NumberOfComponents;
    delete [] this->Tuple;
    this->Tuple = new float[this->TupleSize];
    }

  short *t = this->Array + this->NumberOfComponents*i;
  for (int j=0; j<this->NumberOfComponents; j++) this->Tuple[j] = (float)t[j];
  return this->Tuple;
}

// Description:
// Copy the tuple value into a user-provided array.
void vtkShortArray::GetTuple(const int i, float tuple[]) 
{
  short *t = this->Array + this->NumberOfComponents*i;
  for (int j=0; j<this->NumberOfComponents; j++) tuple[j] = (float)t[j];
}

// Description:
// Set the tuple value at the ith location in the array.
void vtkShortArray::SetTuple(const int i, const float tuple[])
{
  int loc = i * this->NumberOfComponents; 
  for (int j=0; j<this->NumberOfComponents; j++) 
    this->Array[loc+j] = (short)tuple[j];
}

// Description:
// Insert (memory allocation performed) the tuple into the ith location
// in the array.
void vtkShortArray::InsertTuple(const int i, const float tuple[])
{
  short *t = this->WritePointer(i*this->NumberOfComponents,this->NumberOfComponents);

  for (int j=0; j<this->NumberOfComponents; j++) *t++ = (short)*tuple++;
}

// Description:
// Insert (memory allocation performed) the tuple onto the end of the array.
int vtkShortArray::InsertNextTuple(const float tuple[])
{
  int i = this->MaxId + 1;
  short *t = this->WritePointer(i,this->NumberOfComponents);

  for (i=0; i<this->NumberOfComponents; i++) *t++ = (short)*tuple++;

  return this->MaxId / this->NumberOfComponents;
}

// Description:
// Return the data component at the ith tuple and jth component location.
// Note that i<NumberOfTuples and j<NumberOfComponents.
float vtkShortArray::GetComponent(const int i, const int j)
{
  return (float) this->GetValue(i*this->NumberOfComponents + j);
}

// Description:
// Set the data component at the ith tuple and jth component location.
// Note that i<NumberOfTuples and j<NumberOfComponents. Make sure enough
// memory has been allocated (use SetNumberOfTuples() and 
// SetNumberOfComponents()).
void vtkShortArray::SetComponent(const int i, const int j, const float c)
{
  this->SetValue(i*this->NumberOfComponents + j, (short)c);
}

// Description:
// Insert the data component at ith tuple and jth component location. 
// Note that memory allocation is performed as necessary to hold the data.
void vtkShortArray::InsertComponent(const int i, const int j, const float c)
{
  this->InsertValue(i*this->NumberOfComponents + j, (short)c);
}

