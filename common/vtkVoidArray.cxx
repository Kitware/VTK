/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoidArray.cxx
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
#include "vtkVoidArray.h"

typedef void *voidPtr;

// Instantiate object.
vtkVoidArray::vtkVoidArray()
{
  this->Array = NULL;
  this->TupleSize = 3;
  this->Tuple = new float[this->TupleSize]; //used for conversion
}

vtkVoidArray::~vtkVoidArray()
{
  if (this->Array)
    {
    delete [] this->Array;
    }
  delete [] this->Tuple;
}

// Allocate memory for this array. Delete old storage only if necessary.
int vtkVoidArray::Allocate(const int sz, const int ext)
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

  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;

  return 1;
}

// Release storage and reset array to initial state.
void vtkVoidArray::Initialize()
{
  if ( this->Array != NULL )
    {
    delete [] this->Array;
    this->Array = NULL;
    }
  this->Size = 0;
  this->MaxId = -1;
}

// Deep copy of another void array.
void vtkVoidArray::DeepCopy(vtkDataArray& da)
{
  if ( da.GetDataType() != VTK_VOID )
    {
    vtkDataArray::DeepCopy(da);
    return;
    }

  if ( this != &da )
    {
    delete [] this->Array;

    this->MaxId = da.GetMaxId();
    this->Size = da.GetSize();
    this->Extend = da.GetExtend();

    this->Array = new voidPtr[this->Size];
    memcpy(this->Array, da.GetVoidPointer(0), this->Size*sizeof(void *));
    }
}

void vtkVoidArray::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataArray::PrintSelf(os,indent);

  os << indent << "Array: " << this->Array << "\n";
}

// Protected function does "reallocate"
//
void** vtkVoidArray::Resize(const int sz)
{
  void** newArray;
  int newSize;

  if ( sz > this->Size )
    {
    newSize = this->Size + this->Extend*(((sz-this->Size)/this->Extend)+1);
    }
  else if (sz == this->Size)
    {
    return this->Array;
    }
  else
    {
    newSize = sz;
    }

  if ( (newArray = new voidPtr[newSize]) == NULL )
    { 
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  memcpy(newArray, this->Array,
         (sz < this->Size ? sz : this->Size) * sizeof(voidPtr));

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}


// Set the number of n-tuples in the array.
void vtkVoidArray::SetNumberOfTuples(const int number)
{
  this->SetNumberOfValues(number*this->NumberOfComponents);
}

// Get a pointer to a tuple at the ith location.
float *vtkVoidArray::GetTuple(const int vtkNotUsed(i))
{
  return NULL;
}

// Copy the tuple value into a user-provided array.
void vtkVoidArray::GetTuple(const int vtkNotUsed(i), float * vtkNotUsed(tuple))
{
}

// Set the tuple value at the ith location in the array.
void vtkVoidArray::SetTuple(const int vtkNotUsed(i), const float * vtkNotUsed(tuple))
{
}

// Insert (memory allocation performed) the tuple into the ith location
// in the array.
void vtkVoidArray::InsertTuple(const int vtkNotUsed(i), const float * vtkNotUsed(tuple))
{
}

// Insert (memory allocation performed) the tuple onto the end of the array.
int vtkVoidArray::InsertNextTuple(const float * vtkNotUsed(tuple))
{
  return -1;
}


