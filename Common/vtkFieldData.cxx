/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFieldData.cxx
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
#include "vtkFieldData.h"
#include "vtkObjectFactory.h"

vtkFieldData::BasicIterator::BasicIterator(const int* list, 
					   unsigned int listSize)
{
  if (list)
    {
    if (listSize>0)
      {
      this->List = new int[listSize];
      memcpy(this->List, list, listSize*sizeof(int));
      }
    else
      {
      this->List = 0;
      }
    this->ListSize = listSize;
    }
  else
    {
    this->ListSize = 0;
    }
  this->Position = 0;
}

vtkFieldData::Iterator::Iterator(vtkFieldData* dsa, const int* list, 
				 unsigned int listSize)
  : vtkFieldData::BasicIterator(list, listSize)
{
  this->Fields = dsa;
  dsa->Register(0);
  if (!list)
    {
    this->ListSize = dsa->GetNumberOfArrays();
    this->List = new int[this->ListSize];
    for(int i=0; i<this->ListSize; i++) { this->List[i] = i; }
    }
  this->Detached = 0;
}

vtkFieldData::BasicIterator::BasicIterator()
{
  this->List  = 0;
  this->ListSize = 0;
}

vtkFieldData::Iterator::Iterator()
{
  this->Detached = 0;
  this->Fields = 0;
}

vtkFieldData::BasicIterator::BasicIterator(const vtkFieldData::BasicIterator& 
					   source)
{
  this->ListSize = source.ListSize;

  if (this->ListSize > 0)
    {
    this->List = new int[this->ListSize];
    memcpy(this->List, source.List, this->ListSize*sizeof(int));
    }
  else
    {
    this->List = 0;
    }
}

vtkFieldData::Iterator::Iterator(const vtkFieldData::Iterator& source)
  : vtkFieldData::BasicIterator(source)
{
  this->Detached = source.Detached;
  this->Fields = source.Fields;
  if (this->Fields && !this->Detached)
    {
    this->Fields->Register(0);
    }
}

vtkFieldData::BasicIterator& vtkFieldData::BasicIterator::operator=(
  const vtkFieldData::BasicIterator& source)
{
  if (this == &source)
    {
    return *this;
    }
  delete[] this->List;
  this->ListSize = source.ListSize;
  if (this->ListSize > 0)
    {
    this->List = new int[this->ListSize];
    memcpy(this->List, source.List, this->ListSize*sizeof(int));
    }
  else
    {
    this->List = 0;
    }
  return *this;
}

vtkFieldData::Iterator& vtkFieldData::Iterator::operator=(
  const vtkFieldData::Iterator& source)
{
  if (this == &source)
    {
    return *this;
    }
  this->BasicIterator::operator=(source);
  if (this->Fields && !this->Detached)
    {
    this->Fields->UnRegister(0);
    }
  this->Fields = source.Fields;
  this->Detached = source.Detached;
  if (this->Fields && !this->Detached)
    {
    this->Fields->Register(0);
    }
  return *this;
}

vtkFieldData::BasicIterator::~BasicIterator()
{
  delete[] this->List;
}

vtkFieldData::Iterator::~Iterator()
{
  if (this->Fields && !this->Detached)
    {
    this->Fields->UnRegister(0);
    }
}

void vtkFieldData::Iterator::DetachFieldData()
{
  if (this->Fields && !this->Detached)
    {
    this->Fields->UnRegister(0);
    this->Detached = 1;
    }
}

int vtkFieldData::BasicIterator::IsInList(int index)
{
  for(int i=0; i<this->ListSize; i++)
    {
    if (index == this->List[i])
      {
      return 1;
      }
    }
  return 0;
}


//------------------------------------------------------------------------------
vtkFieldData* vtkFieldData::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkFieldData");
  if(ret)
    {
    return (vtkFieldData*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkFieldData;
}


// Construct object with no data initially.
vtkFieldData::vtkFieldData()
{
  this->NumberOfArrays = 0;
  this->Data = NULL;
  
  this->TupleSize = 0;
  this->Tuple = NULL;

  this->NumberOfActiveArrays = 0;
}

vtkFieldData::~vtkFieldData()
{
  this->Initialize();
  delete[] this->Tuple;
}

// Release all data but do not delete object.
void vtkFieldData::Initialize()
{
  int i;

  if ( this->Data )
    {
    for ( i=0; i<this->GetNumberOfArrays(); i++ )
      {
      this->Data[i]->UnRegister(this);
      }
    
    delete [] this->Data;
    this->Data = NULL;
    }

  this->NumberOfArrays = 0;
  this->NumberOfActiveArrays = 0;
}

// Allocate data for each array.
int vtkFieldData::Allocate(const int sz, const int ext)
{
  int i;
  int status = 0;
  
  for ( i=0; i < this->GetNumberOfArrays(); i++ )
    {
    if ( (status = this->Data[i]->Allocate(sz,ext)) == 0 )
      {
      break;
      }
    }

  return status;
}

// Virtual constructor creates a field with the same number of data arrays and
// types of data arrays, but the arrays contain nothing.
vtkFieldData *vtkFieldData::MakeObject()
{
  vtkFieldData *f= vtkFieldData::New();
  int i;
  vtkDataArray *data;
  
  f->SetNumberOfArrays(this->GetNumberOfArrays());
  for ( i=0; i < this->GetNumberOfArrays(); i++ )
    {
    data = this->Data[i]->MakeObject();
    data->SetName(this->Data[i]->GetName());
    f->SetArray(i, data);
    data->Delete();
    }

  return f;
}

// Set the number of arrays used to define the field.
void vtkFieldData::SetNumberOfArrays(int num)
{
  int i;
  
  if ( num < 0 )
    {
    num = 0;
    }

  if ( num == this->NumberOfArrays )
    {
    return;
    }
  else
    {
    this->Modified();
    }

  if ( num == 0 ) 
    {
    this->Initialize();
    }
  else if ( num < this->NumberOfArrays )
    {
    for ( i=num; i < this->NumberOfArrays; i++)
      {
      this->Data[i]->UnRegister(this);
      }
    this->NumberOfArrays = num;
    }
  else //num > this->NumberOfArrays
    {
    vtkDataArray **data=new vtkDataArray * [num];
    // copy the original data
    for ( i=0; i < this->NumberOfArrays; i++ )
      {
      data[i] = this->Data[i];
      }

    //initialize the new arrays
    for ( i=this->NumberOfArrays; i < num; i++ )
      {
      data[i] = 0;
      }

    // get rid of the old data
    delete [] this->Data;
    
    // update object
    this->Data = data;
    this->NumberOfArrays = num;
    }
}

// Set an array to define the field.
void vtkFieldData::SetArray(int i, vtkDataArray *data)
{
  if (!data || (i > this->NumberOfActiveArrays))
    {
    vtkWarningMacro("Can not set array " << i << " to " << data << endl);
    return;
    }

  int numComp;

  if ( i < 0 )
    {
    vtkWarningMacro("Array index should be >= 0");
    return;
    }
  else if (i >= this->NumberOfArrays)
    {
    this->SetNumberOfArrays(i+1);
    this->NumberOfActiveArrays = i+1;
    }

  if ( this->Data[i] != data )
    {
    this->Modified();
    if ( this->Data[i] != NULL )
      {
      this->Data[i]->UnRegister(this);
      }
    this->Data[i] = data;
    if ( this->Data[i] != NULL )
      {
      this->Data[i]->Register(this);
      }
    }
  
  // adjust scratch tuple array
  numComp = this->GetNumberOfComponents();
  if ( numComp != this->TupleSize )
    {
    this->TupleSize = numComp;
    if ( this->Tuple )
      {
      delete [] this->Tuple;
      }
    this->Tuple = new float[this->TupleSize];
    }
}

// 
int vtkFieldData::GetNumberOfArrays()
{
  return this->NumberOfActiveArrays;
}

// Return the ith array in the field. A NULL is returned if the index i is out
// if range.
vtkDataArray *vtkFieldData::GetArray(int i)
{
  if ( i < 0 || i >= this->GetNumberOfArrays() )
    {
    return 0;
    }
  return this->Data[i];
}

// Copy a field by reference counting the data arrays.
void vtkFieldData::DeepCopy(vtkFieldData *f)
{
  vtkDataArray *data, *newData;

  this->SetNumberOfArrays(f->GetNumberOfArrays());
  for ( int i=0; i < f->GetNumberOfArrays(); i++ )
    {
    data = f->GetArray(i);
    newData = data->MakeObject(); //instantiate same type of object
    newData->DeepCopy(data);
    newData->SetName(data->GetName());
    this->AddArray(newData);
    newData->Delete();
    }
}

// Copy a field by reference counting the data arrays.
void vtkFieldData::ShallowCopy(vtkFieldData *f)
{
  this->SetNumberOfArrays(f->GetNumberOfArrays());
  this->NumberOfActiveArrays = 0;

  for ( int i=0; i < f->GetNumberOfArrays(); i++ )
    {
    this->NumberOfActiveArrays++;
    this->SetArray(i, f->GetArray(i));
    }
}


// Squeezes each data array in the field (Squeeze() reclaims unused memory.)
void vtkFieldData::Squeeze()
{
  for ( int i=0; i < this->GetNumberOfArrays(); i++ )
    {
    this->Data[i]->Squeeze();
    }
}

// Resets each data array in the field (Reset() does not release memory but
// it makes the arrays look like they are empty.)
void vtkFieldData::Reset()
{
  int i;

  for ( i=0; i < this->GetNumberOfArrays(); i++ )
    {
    this->Data[i]->Reset();
    }
}

// Get a field from a list of ids. Supplied field f should have same types
// and number of data arrays as this one (i.e., like MakeObject() returns).
void vtkFieldData::GetField(vtkIdList *ptIds, vtkFieldData *f)
{
  int i, numIds=ptIds->GetNumberOfIds();

  for (i=0; i < numIds; i++)
    {
    f->InsertTuple(i, this->GetTuple(ptIds->GetId(i)));
    }
}

// Return the array containing the ith component of the field. The return value
// is an integer number n 0<=n<this->NumberOfArrays. Also, an integer value is
// returned indicating the component in the array is returned. Method returns
// -1 if specified component is not in field.
int vtkFieldData::GetArrayContainingComponent(int i, int& arrayComp)
{
  int numComp, count=0;

  for ( int j=0; j < this->GetNumberOfArrays(); j++ )
    {
    if ( this->Data[j] != NULL )
      {
      numComp = this->Data[j]->GetNumberOfComponents();
      if ( j < (numComp + count) )
        {
        arrayComp = i - count;
        return j;
        }
      count += numComp;
      }
    }
  return -1;
}

vtkDataArray *vtkFieldData::GetArray(const char *arrayName)
{
  int i;
  return this->GetArray(arrayName, i);
}

vtkDataArray *vtkFieldData::GetArray(const char *arrayName, int &index)
{
  int i;
  const char *name;
  index = -1;
  for (i=0; i < this->GetNumberOfArrays(); i++)
    {
    name = this->GetArrayName(i);
    if ( !strcmp(name,arrayName) )
      {
      index = i;
      return this->GetArray(i);
      }
    }
  return NULL;
}

int vtkFieldData::AddArray(vtkDataArray *array)
{
  if (!array)
    {
    return -1;
    }

  int index;
  this->GetArray(array->GetName(), index);

  if (index == -1)
    {
    index = this->NumberOfActiveArrays;
    this->NumberOfActiveArrays++;
    }
  this->SetArray(index, array);
  return index;
}

void vtkFieldData::RemoveArray(int index)
{
  if ( (index<0) || (index>=this->NumberOfActiveArrays))
    {
    return;
    }
  this->Data[index]->UnRegister(this);
  this->Data[index] = 0;
  for(int i=index; i<this->GetNumberOfArrays()-1; i++)
    {
    this->Data[i] = this->Data[i+1];
    }
  this->Data[this->GetNumberOfArrays()-1] = 0;
  this->NumberOfActiveArrays--;
}

unsigned long vtkFieldData::GetActualMemorySize()
{
  unsigned long size=0;

  for ( int i=0; i < this->GetNumberOfArrays(); i++ )
    {
    if ( this->Data[i] != NULL )
      {
      size += this->Data[i]->GetActualMemorySize();
      }
    }

  return size;
}

unsigned long int vtkFieldData::GetMTime()
{
  unsigned long int mTime = this->MTime;
  unsigned long int otherMTime;

  if ( this->NumberOfActiveArrays > 0 )
    {
    vtkFieldData::Iterator it(this);
    vtkDataArray* da;
    for(da=it.Begin(); !it.End(); da=it.Next())
      {
      if (da)
	{
	otherMTime = da->GetMTime();
	if ( otherMTime > mTime )
	  {
	  mTime = otherMTime;
	  }
	}
      }
    }
  return mTime;
}

void vtkFieldData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Number Of Arrays: " << this->GetNumberOfArrays() << "\n";
  for (int i=0; i<this->GetNumberOfArrays(); i++)
    {
    os << indent << "Array " << i << " name = " 
       << this->GetArrayName(i) << "\n";
    }
  os << indent << "Number Of Components: " << this->GetNumberOfComponents() 
     << "\n";
  os << indent << "Number Of Tuples: " << this->GetNumberOfTuples() << "\n";
}

// Get the number of components in the field. This is determined by adding
// up the components in each non-NULL array.
int vtkFieldData::GetNumberOfComponents()
{
  int i, numComp;

  for ( i=numComp=0; i < this->GetNumberOfArrays(); i++ )
    {
    numComp += this->Data[i]->GetNumberOfComponents();
    }

  return numComp;
}

// Get the number of tuples in the field.
int vtkFieldData::GetNumberOfTuples()
{
  vtkDataArray* da;
  if ((da=this->GetArray(0)))
    {
    return da->GetNumberOfTuples(); 
    }
  else
    {
    return 0;
    }
}

// Set the number of tuples for each data array in the field.
void vtkFieldData::SetNumberOfTuples(const int number)
{
  for ( int i=0; i < this->GetNumberOfArrays(); i++ )
    {
    this->Data[i]->SetNumberOfTuples(number);
    }
}

// Return a tuple consisting of a concatentation of all data from all
// the different arrays. Note that everything is converted to and from
// float values.
float *vtkFieldData::GetTuple(const int i)
{
  int count=0;

  for ( int j=0; j < this->GetNumberOfArrays(); j++ )
    {
    this->Data[j]->GetTuple(i, this->Tuple + count);
    count += this->Data[j]->GetNumberOfComponents();
    }

  return this->Tuple;
}

// Copy the ith tuple value into a user provided tuple array. Make
// sure that you've allocated enough space for the copy.
void vtkFieldData::GetTuple(const int i, float * tuple)
{
  this->GetTuple(i); //side effect fills in this->Tuple
  for (int j=0; j<this->TupleSize; j++)
    {
    tuple[j] = this->Tuple[j];
    }
}

// Set the tuple value at the ith location. Set operations
// mean that no range chaecking is performed, so they're faster.
void vtkFieldData::SetTuple(const int i, const float * tuple)
{
  int count=0;

  for ( int j=0; j < this->GetNumberOfArrays(); j++ )
    {
    this->Data[j]->SetTuple(i, tuple + count);
    count += this->Data[j]->GetNumberOfComponents();
    }
}

// Insert the tuple value at the ith location. Range checking is
// performed and memory allocates as necessary.
void vtkFieldData::InsertTuple(const int i, const float * tuple)
{
  int count=0;

  for ( int j=0; j < this->GetNumberOfArrays(); j++ )
    {
    this->Data[j]->InsertTuple(i, tuple + count);
    count += this->Data[j]->GetNumberOfComponents();
    }
}

// Insert the tuple value at the end of the tuple matrix. Range
// checking is performed and memory is allocated as necessary.
int vtkFieldData::InsertNextTuple(const float * tuple)
{
  int id=this->GetNumberOfTuples();

  this->InsertTuple(id, tuple);
  return id;
}

// Get the component value at the ith tuple (or row) and jth component (or column).
float vtkFieldData::GetComponent(const int i, const int j)
{
  this->GetTuple(i);
  return this->Tuple[j];
}

// Set the component value at the ith tuple (or row) and jth component (or column).
// Range checking is not performed, so set the object up properly before invoking.
void vtkFieldData::SetComponent(const int i, const int j, const float c)
{
  this->GetTuple(i);
  this->Tuple[j] = c;
  this->SetTuple(i,this->Tuple);
}

// Insert the component value at the ith tuple (or row) and jth component (or column).
// Range checking is performed and memory allocated as necessary o hold data.
void vtkFieldData::InsertComponent(const int i, const int j, const float c)
{
  this->GetTuple(i);
  this->Tuple[j] = c;
  this->InsertTuple(i,this->Tuple);
}

