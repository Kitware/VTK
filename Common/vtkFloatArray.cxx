/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatArray.cxx
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
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkFloatArray* vtkFloatArray::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkFloatArray");
  if(ret)
    {
    return (vtkFloatArray*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkFloatArray;
}

vtkDataArray *vtkFloatArray::MakeObject()
{
  vtkDataArray *a = vtkFloatArray::New();
  a->SetNumberOfComponents(this->NumberOfComponents);
  return a;
}

// Instantiate object with 1 components.
vtkFloatArray::vtkFloatArray(vtkIdType numComp)
{
  this->NumberOfComponents = (numComp < 1 ? 1 : numComp);
  this->Array = NULL;
  this->SaveUserArray = 0;
}

vtkFloatArray::~vtkFloatArray()
{
  if ((this->Array) && (!this->SaveUserArray))
    {
    delete [] this->Array;
    }

}

// This method lets the user specify data to be held by the array.  The 
// array argument is a pointer to the data.  size is the size of 
// the array supplied by the user.  Set save to 1 to keep the class
// from deleting the array when it cleans up or reallocates memory.
// The class uses the actual array provided; it does not copy the data 
// from the suppled array.
void vtkFloatArray::SetArray(float* array, vtkIdType size, int save)
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


// Allocate memory for this array. Delete old storage only if necessary.
int vtkFloatArray::Allocate(const vtkIdType sz,
                            const vtkIdType vtkNotUsed(ext))
{
  if ( sz > this->Size)
    {
    if ((this->Array) && (!this->SaveUserArray))
      {
      delete [] this->Array;
      }
    this->Size = ( sz > 0 ? sz : 1);
    if ( (this->Array = new float[this->Size]) == NULL )
      {
      return 0;
      }
    this->SaveUserArray = 0;
    }

  this->MaxId = -1;

  return 1;
}

// Release storage and reset array to initial state.
void vtkFloatArray::Initialize()
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

// Deep copy of another float array.
void vtkFloatArray::DeepCopy(vtkDataArray *fa)
{
  if ( fa->GetDataType() != VTK_FLOAT )
    {
    vtkDataArray::DeepCopy(fa);
    return;
    }

  if ( this != fa )
    {
    if ((this->Array) && (!this->SaveUserArray))
      {
      delete [] this->Array;
      }

    this->NumberOfComponents = fa->GetNumberOfComponents();
    this->MaxId = fa->GetMaxId();
    this->Size = fa->GetSize();
    this->SaveUserArray = 0;

    this->Array = new float[this->Size];
    memcpy(this->Array, (float *)fa->GetVoidPointer(0), this->Size*sizeof(float));
    }
}

void vtkFloatArray::PrintSelf(ostream& os, vtkIndent indent)
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
float *vtkFloatArray::ResizeAndExtend(const vtkIdType sz)
{
  float *newArray;
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

  if ( (newArray = new float[newSize]) == NULL )
    { 
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  if (this->Array)
    {
    memcpy(newArray, this->Array,
	   (sz < this->Size ? sz : this->Size) * sizeof(float));
    if (!this->SaveUserArray)
      {
      delete [] this->Array;
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

void vtkFloatArray::Resize(vtkIdType sz)
{
  float *newArray;
  vtkIdType newSize = sz * this->NumberOfComponents;

  if (newSize == this->Size)
    {
    return;
    }

  if (newSize <= 0)
    {
    this->Initialize();
    return;
    }

  if ( (newArray = new float[newSize]) == NULL )
    { 
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return;
    }

  if (this->Array)
    {
    memcpy(newArray, this->Array,
	   (newSize < this->Size ? newSize : this->Size) * sizeof(float));
    if (!this->SaveUserArray)
      {
      delete [] this->Array;
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
void vtkFloatArray::SetNumberOfTuples(const vtkIdType number)
{
  this->SetNumberOfValues(number*this->NumberOfComponents);
}

// Get a pointer to a tuple at the ith location.
float *vtkFloatArray::GetTuple(const vtkIdType i)
{
  return this->Array + this->NumberOfComponents*i;
}

// Copy the tuple value into a user-provided array.
void vtkFloatArray::GetTuple(const vtkIdType i, float * tuple)
{
  float *t = this->Array + this->NumberOfComponents*i;
  for (int j=0; j<this->NumberOfComponents; j++)
    {
    tuple[j] = t[j];
    }
}

void vtkFloatArray::GetTuple(const vtkIdType i, double * tuple) 
{
  float *t = this->Array + this->NumberOfComponents*i;
  for (int j=0; j<this->NumberOfComponents; j++)
    {
    tuple[j] = (double)t[j];
    }
}

// Set the tuple value at the ith location in the array.
void vtkFloatArray::SetTuple(const vtkIdType i, const float * tuple)
{
  vtkIdType loc = i * this->NumberOfComponents; 
  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->Array[loc+j] = tuple[j];
    }
}

void vtkFloatArray::SetTuple(const vtkIdType i, const double * tuple)
{
  vtkIdType loc = i * this->NumberOfComponents; 
  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->Array[loc+j] = (float)tuple[j];
    }
}

// Insert (memory allocation performed) the tuple into the ith location
// in the array.
void vtkFloatArray::InsertTuple(const vtkIdType i, const float * tuple)
{
  float *t = this->WritePointer(i*this->NumberOfComponents,this->NumberOfComponents);

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    *t++ = *tuple++;
    }
}

void vtkFloatArray::InsertTuple(const vtkIdType i, const double * tuple)
{
  float *t = this->WritePointer(i*this->NumberOfComponents,this->NumberOfComponents);

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    *t++ = (float)*tuple++;
    }
}

// Insert (memory allocation performed) the tuple onto the end of the array.
vtkIdType vtkFloatArray::InsertNextTuple(const float * tuple)
{
  vtkIdType i = this->MaxId + 1;
  float *t = this->WritePointer(i,this->NumberOfComponents);

  for (i=0; i<this->NumberOfComponents; i++)
    {
    *t++ = *tuple++;
    }

  return this->MaxId / this->NumberOfComponents;

}

vtkIdType vtkFloatArray::InsertNextTuple(const double * tuple)
{
  vtkIdType i = this->MaxId + 1;
  float *t = this->WritePointer(i,this->NumberOfComponents);

  for (i=0; i<this->NumberOfComponents; i++)
    {
    *t++ = (float)*tuple++;
    }

  return this->MaxId / this->NumberOfComponents;
}

// Return the data component at the ith tuple and jth component location.
// Note that i<NumberOfTuples and j<NumberOfComponents.
float vtkFloatArray::GetComponent(const vtkIdType i, const int j)
{
  return this->GetValue(i*this->NumberOfComponents + j);
}

// Set the data component at the ith tuple and jth component location.
// Note that i<NumberOfTuples and j<NumberOfComponents. Make sure enough
// memory has been allocated (use SetNumberOfTuples() and 
// SetNumberOfComponents()).
void vtkFloatArray::SetComponent(const vtkIdType i, const int j, const float c)
{
  this->SetValue(i*this->NumberOfComponents + j, c);
}

// Insert the data component at ith tuple and jth component location. 
// Note that memory allocation is performed as necessary to hold the data.
void vtkFloatArray::InsertComponent(const vtkIdType i, const int j,
                                    const float c)
{
  this->InsertValue(i*this->NumberOfComponents + j, c);
}


