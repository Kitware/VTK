/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBitArray.cxx
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
#include "vtkBitArray.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkBitArray* vtkBitArray::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkBitArray");
  if(ret)
    {
    return (vtkBitArray*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkBitArray;
}

vtkDataArray *vtkBitArray::MakeObject()
{
  vtkDataArray *a = vtkBitArray::New();
  a->SetNumberOfComponents(this->NumberOfComponents);
  return a;
}

// Instantiate object.
vtkBitArray::vtkBitArray(int numComp)
{
  this->NumberOfComponents = (numComp < 1 ? 1 : numComp);
  this->Array = NULL;
  this->TupleSize = 3;
  this->Tuple = new float[this->TupleSize]; //used for conversion
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

// This method lets the user specify data to be held by the array.  The 
// array argument is a pointer to the data.  size is the size of 
// the array supplied by the user.  Set save to 1 to keep the class
// from deleting the array when it cleans up or reallocates memory.
// The class uses the actual array provided; it does not copy the data 
// from the suppled array.
void vtkBitArray::SetArray(unsigned char* array, int size, int save)
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
int vtkBitArray::GetValue(const int id)
{
  if (this->Array[id/8]&(0x80 >> (id%8)))
    {
    return 1;
    }
  return 0;
}

// Allocate memory for this array. Delete old storage only if necessary.
int vtkBitArray::Allocate(const int sz, const int vtkNotUsed(ext))
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
  if (ia->GetDataType() != VTK_BIT)
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

    this->Array = new unsigned char[(this->Size+7)/8];
    memcpy(this->Array, (unsigned char*)ia->GetVoidPointer(0),
	   ((this->Size+7)/8)*sizeof(unsigned char));
    }
}

void vtkBitArray::PrintSelf(ostream& os, vtkIndent indent)
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

//
// Private function does "reallocate". Sz is the number of "bits", and we
// can allocate only 8-bit bytes.
unsigned char *vtkBitArray::ResizeAndExtend(const int sz)
{
  unsigned char *newArray;
  int newSize;

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

void vtkBitArray::Resize(int sz)
{
  unsigned char *newArray;
  int newSize = sz*this->NumberOfComponents;

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
void vtkBitArray::SetNumberOfTuples(const int number)
{
  this->SetNumberOfValues(number*this->NumberOfComponents);
}

// Get a pointer to a tuple at the ith location. This is a dangerous method
// (it is not thread safe since a pointer is returned).
float *vtkBitArray::GetTuple(const int i)
{
  if ( this->TupleSize < this->NumberOfComponents )
    {
    this->TupleSize = this->NumberOfComponents;
    delete [] this->Tuple;
    this->Tuple = new float[this->TupleSize];
    }

  int loc = this->NumberOfComponents*i;
  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->Tuple[j] = (float)this->GetValue(loc+j);
    }

  return this->Tuple;
}

// Copy the tuple value into a user-provided array.
void vtkBitArray::GetTuple(const int i, float * tuple)
{
  int loc = this->NumberOfComponents*i;

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    tuple[j] = (float)this->GetValue(loc+j);
    }
}

void vtkBitArray::GetTuple(const int i, double * tuple)
{
  int loc = this->NumberOfComponents*i;

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    tuple[j] = (double)this->GetValue(loc+j);
    }
}

// Set the tuple value at the ith location in the array.
void vtkBitArray::SetTuple(const int i, const float * tuple)
{
  int loc = i * this->NumberOfComponents; 

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->SetValue(loc+j,(int)tuple[j]);
    }
}

void vtkBitArray::SetTuple(const int i, const double * tuple)
{
  int loc = i * this->NumberOfComponents; 

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->SetValue(loc+j,(int)tuple[j]);
    }
}

// Insert (memory allocation performed) the tuple into the ith location
// in the array.
void vtkBitArray::InsertTuple(const int i, const float * tuple)
{
  int loc = this->NumberOfComponents*i;

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->InsertValue(loc+j,(int)tuple[j]);
    }
}

void vtkBitArray::InsertTuple(const int i, const double * tuple)
{
  int loc = this->NumberOfComponents*i;

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->InsertValue(loc+j,(int)tuple[j]);
    }
}

// Insert (memory allocation performed) the tuple onto the end of the array.
int vtkBitArray::InsertNextTuple(const float * tuple)
{
  for (int i=0; i<this->NumberOfComponents; i++)
    {
    this->InsertNextValue((int)tuple[i]);
    }

  return this->MaxId / this->NumberOfComponents;
}

int vtkBitArray::InsertNextTuple(const double * tuple)
{
  for (int i=0; i<this->NumberOfComponents; i++)
    {
    this->InsertNextValue((int)tuple[i]);
    }

  return this->MaxId / this->NumberOfComponents;
}

