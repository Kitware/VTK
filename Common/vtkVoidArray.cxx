/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoidArray.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkVoidArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkVoidArray* vtkVoidArray::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkVoidArray");
  if(ret)
    {
    return (vtkVoidArray*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkVoidArray;
}




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
int vtkVoidArray::Allocate(const vtkIdType sz, const int vtkNotUsed(ext))
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
void vtkVoidArray::DeepCopy(vtkDataArray *da)
{
  if ( da->GetDataType() != VTK_VOID )
    {
    vtkDataArray::DeepCopy(da);
    return;
    }

  if ( this != da )
    {
    delete [] this->Array;

    this->MaxId = da->GetMaxId();
    this->Size = da->GetSize();

    this->Array = new voidPtr[this->Size];
    memcpy(this->Array, da->GetVoidPointer(0), this->Size*sizeof(void *));
    }
}

void vtkVoidArray::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataArray::PrintSelf(os,indent);

  if (this->Array)
    {
    os << indent << "Array: " << this->Array << "\n";
    }
  else
    {
    os << indent << "Array: (null)\n";
    }
}

// Protected function does "reallocate"
//
void** vtkVoidArray::ResizeAndExtend(const vtkIdType sz)
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
    this->MaxId = newSize-1;
    }
  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

void vtkVoidArray::Resize(vtkIdType sz)
{
  void** newArray;
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

  if ( (newArray = new voidPtr[newSize]) == NULL )
    { 
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return;
    }

  memcpy(newArray, this->Array,
         (newSize < this->Size ? newSize : this->Size) * sizeof(voidPtr));

  if (newSize < this->Size)
    {
    this->MaxId = newSize-1;
    }
  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return;
}

// Set the number of n-tuples in the array.
void vtkVoidArray::SetNumberOfTuples(const vtkIdType number)
{
  this->SetNumberOfValues(number*this->NumberOfComponents);
}

// Get a pointer to a tuple at the ith location.
float *vtkVoidArray::GetTuple(const vtkIdType vtkNotUsed(i))
{
  return NULL;
}

// Copy the tuple value into a user-provided array.
void vtkVoidArray::GetTuple(const vtkIdType vtkNotUsed(i),
                            float * vtkNotUsed(tuple))
{
}

void vtkVoidArray::GetTuple(const vtkIdType vtkNotUsed(i),
                            double * vtkNotUsed(tuple))
{
}

// Set the tuple value at the ith location in the array.
void vtkVoidArray::SetTuple(const vtkIdType vtkNotUsed(i),
                            const float * vtkNotUsed(tuple))
{
}

void vtkVoidArray::SetTuple(const vtkIdType vtkNotUsed(i),
                            const double * vtkNotUsed(tuple))
{
}

// Insert (memory allocation performed) the tuple into the ith location
// in the array.
void vtkVoidArray::InsertTuple(const vtkIdType vtkNotUsed(i),
                               const float * vtkNotUsed(tuple))
{
}

void vtkVoidArray::InsertTuple(const vtkIdType vtkNotUsed(i),
                               const double * vtkNotUsed(tuple))
{
}

// Insert (memory allocation performed) the tuple onto the end of the array.
int vtkVoidArray::InsertNextTuple(const float * vtkNotUsed(tuple))
{
  return -1;
}

int vtkVoidArray::InsertNextTuple(const double * vtkNotUsed(tuple))
{
  return -1;
}


