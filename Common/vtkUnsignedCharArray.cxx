/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsignedCharArray.cxx
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
#include "vtkUnsignedCharArray.h"
#include "vtkObjectFactory.h"

//---------------------------------------------------------------------------
vtkUnsignedCharArray* vtkUnsignedCharArray::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkUnsignedCharArray");
  if(ret)
    {
    return (vtkUnsignedCharArray*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkUnsignedCharArray;
}

vtkDataArray *vtkUnsignedCharArray::MakeObject()
{
  vtkDataArray *a = vtkUnsignedCharArray::New();
  a->SetNumberOfComponents(this->NumberOfComponents);
  return a;
}

// Instantiate object.
vtkUnsignedCharArray::vtkUnsignedCharArray(vtkIdType numComp)
{
  this->NumberOfComponents = (numComp < 1 ? 1 : numComp);
  this->Array = NULL;
  this->TupleSize = 3;
  this->Tuple = new float[this->TupleSize]; //used for conversion
  this->SaveUserArray = 0;  
}

vtkUnsignedCharArray::~vtkUnsignedCharArray()
{
  if ((this->Array) && (!this->SaveUserArray))
    {
    delete [] this->Array;
    }
  delete [] this->Tuple;
}

// This method lets the user specify data to be held by the array.  The 
// array argument is a pointer to the data.  size is the size of 
// the array supplied by the user.  Set save to 1 to keep the class
// from deleting the array when it cleans up or reallocates memory.
// The class uses the actual array provided; it does not copy the data 
// from the suppled array.
void vtkUnsignedCharArray::SetArray(unsigned char* array, vtkIdType size,
                                    int save)
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
int vtkUnsignedCharArray::Allocate(const vtkIdType sz,
                                   const vtkIdType vtkNotUsed(ext))
{
  if ( sz > this->Size )
    {
    if ((this->Array) && (!this->SaveUserArray))
      {
      delete [] this->Array;
      }
    this->Size = ( sz > 0 ? sz : 1);
    if ( (this->Array = new unsigned char[this->Size]) == NULL )
      {
      return 0;
      }
    this->SaveUserArray = 0;
    }

  this->MaxId = -1;

  return 1;
}

// Release storage and reset array to initial state.
void vtkUnsignedCharArray::Initialize()
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

// Deep copy of another unsigned char array.
void vtkUnsignedCharArray::DeepCopy(vtkDataArray *ia)
{
  if ( ia->GetDataType() != VTK_UNSIGNED_CHAR )
    {
      vtkDataArray::DeepCopy(ia);
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

    this->Array = new unsigned char[this->Size];
    memcpy(this->Array, (unsigned char *)ia->GetVoidPointer(0),
           this->Size*sizeof(unsigned char));

    }
}

void vtkUnsignedCharArray::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataArray::PrintSelf(os,indent);

  if (this->Array)
    {
    os << indent << "Array: " << (void *) this->Array << "\n";
    }
  else
    {
    os << indent << "Array: (null)\n";
    }
}

//
// Private function does "reallocate"
//
unsigned char *vtkUnsignedCharArray::ResizeAndExtend(const vtkIdType sz)
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

  if ( (newArray = new unsigned char[newSize]) == NULL )
    {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }


  if (this->Array)
    {
    memcpy(newArray, this->Array, 
	   (sz < this->Size ? sz : this->Size) * sizeof(char));
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

void vtkUnsignedCharArray::Resize(vtkIdType sz)
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

  if ( (newArray = new unsigned char[newSize]) == NULL )
    {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return;
    }


  if (this->Array)
    {
    memcpy(newArray, this->Array, 
	   (newSize < this->Size ? newSize : this->Size) * sizeof(char));
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
void vtkUnsignedCharArray::SetNumberOfTuples(const vtkIdType number)
{
  this->SetNumberOfValues(number*this->NumberOfComponents);
}

// Get a pointer to a tuple at the ith location. This is a dangerous method
// (it is not thread safe since a pointer is returned).
float *vtkUnsignedCharArray::GetTuple(const vtkIdType i) 
{
  if ( this->TupleSize < this->NumberOfComponents )
    {
    this->TupleSize = this->NumberOfComponents;
    delete [] this->Tuple;
    this->Tuple = new float[this->TupleSize];
    }

  unsigned char *t = this->Array + this->NumberOfComponents*i;
  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->Tuple[j] = (float)t[j];
    }
  return this->Tuple;
}

// Copy the tuple value into a user-provided array.
void vtkUnsignedCharArray::GetTuple(const vtkIdType i, float * tuple) 
{
  unsigned char *t = this->Array + this->NumberOfComponents*i;
  for (int j=0; j<this->NumberOfComponents; j++)
    {
    tuple[j] = (float)t[j];
    }
}

void vtkUnsignedCharArray::GetTuple(const vtkIdType i, double * tuple) 
{
  unsigned char *t = this->Array + this->NumberOfComponents*i;
  for (int j=0; j<this->NumberOfComponents; j++)
    {
    tuple[j] = (double)t[j];
    }
}

// Set the tuple value at the ith location in the array.
void vtkUnsignedCharArray::SetTuple(const vtkIdType i, const float * tuple)
{
  vtkIdType loc = i * this->NumberOfComponents; 
  for (int j=0; j<this->NumberOfComponents; j++) 
    {
    this->Array[loc+j] = (unsigned char)tuple[j];
    }
}

void vtkUnsignedCharArray::SetTuple(const vtkIdType i, const double * tuple)
{
  vtkIdType loc = i * this->NumberOfComponents; 
  for (int j=0; j<this->NumberOfComponents; j++) 
    {
    this->Array[loc+j] = (unsigned char)tuple[j];
    }
}

// Insert (memory allocation performed) the tuple into the ith location
// in the array.
void vtkUnsignedCharArray::InsertTuple(const vtkIdType i, const float * tuple)
{
  unsigned char *t = this->WritePointer(i*this->NumberOfComponents,this->NumberOfComponents);

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    *t++ = (unsigned char)*tuple++;
    }
}

void vtkUnsignedCharArray::InsertTuple(const vtkIdType i, const double * tuple)
{
  unsigned char *t = this->WritePointer(i*this->NumberOfComponents,this->NumberOfComponents);

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    *t++ = (unsigned char)*tuple++;
    }
}

// Insert (memory allocation performed) the tuple onto the end of the array.
vtkIdType vtkUnsignedCharArray::InsertNextTuple(const float * tuple)
{
  vtkIdType i = this->MaxId + 1;
  unsigned char *t = this->WritePointer(i,this->NumberOfComponents);

  for (i=0; i<this->NumberOfComponents; i++)
    {
    *t++ = (unsigned char)*tuple++;
    }

  return this->MaxId / this->NumberOfComponents;
}

vtkIdType vtkUnsignedCharArray::InsertNextTuple(const double * tuple)
{
  vtkIdType i = this->MaxId + 1;
  unsigned char *t = this->WritePointer(i,this->NumberOfComponents);

  for (i=0; i<this->NumberOfComponents; i++)
    {
    *t++ = (unsigned char)*tuple++;
    }

  return this->MaxId / this->NumberOfComponents;
}

// Return the data component at the ith tuple and jth component location.
// Note that i<NumberOfTuples and j<NumberOfComponents.
float vtkUnsignedCharArray::GetComponent(const vtkIdType i, const int j)
{
  return (float) this->GetValue(i*this->NumberOfComponents + j);
}

// Set the data component at the ith tuple and jth component location.
// Note that i<NumberOfTuples and j<NumberOfComponents. Make sure enough
// memory has been allocated (use SetNumberOfTuples() and 
// SetNumberOfComponents()).
void vtkUnsignedCharArray::SetComponent(const vtkIdType i, const int j,
                                        const float c)
{
  this->SetValue(i*this->NumberOfComponents + j, (unsigned char)c);
}

// Insert the data component at ith tuple and jth component location. 
// Note that memory allocation is performed as necessary to hold the data.
void vtkUnsignedCharArray::InsertComponent(const vtkIdType i, const int j,
                                           const float c)
{
  this->InsertValue(i*this->NumberOfComponents + j, (unsigned char)c);
}

