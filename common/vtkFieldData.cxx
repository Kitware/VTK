/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFieldData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkFieldData.h"
#include "vtkObjectFactory.h"



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

  this->ArrayNames = NULL;
}

vtkFieldData::~vtkFieldData()
{
  this->Initialize();
  if ( this->Tuple )
    {
    delete [] this->Tuple;
    }
}

// Release all data but do not delete object.
void vtkFieldData::Initialize()
{
  int i;

  if (this->ArrayNames)
    {
    for ( i=0; i < this->NumberOfArrays; i++ )
      {
      if (this->ArrayNames[i] != NULL)
        {
        delete [] this->ArrayNames[i];
        }
      }

    delete [] this->ArrayNames;
    this->ArrayNames = NULL;
    }

  if ( this->Data )
    {
    for ( i=0; i<this->NumberOfArrays; i++ )
      {
      if ( this->Data[i] != NULL ) 
        {
        this->Data[i]->UnRegister(this);
        }
      }
    
    delete [] this->Data;
    this->Data = NULL;
    }

  this->NumberOfArrays = 0;
}

// Allocate data for each array.
int vtkFieldData::Allocate(const int sz, const int ext)
{
  int i, status;
  
  for ( i=0; i < this->NumberOfArrays; i++ )
    {
    if ( this->Data[i] != NULL )
      {
      if ( (status = this->Data[i]->Allocate(sz,ext)) == 0 )
        {
        break;
        }
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
  
  f->SetNumberOfArrays(this->NumberOfArrays);
  for ( i=0; i < this->NumberOfArrays; i++ )
    {
    if ( this->Data[i] != NULL )
      {
      data = this->Data[i]->MakeObject();
      f->SetArray(i,data);
      data->Delete();
      f->SetArrayName(i,this->GetArrayName(i));
      }
    }

  return f;
}


// Set the number of arrays used to define the field.
void vtkFieldData::SetNumberOfArrays(int num)
{
  int i;
  char **arrayNames;
  
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
      delete [] this->ArrayNames[i];
      }
    this->NumberOfArrays = num;
    }

  else //num > this->NumberOfArrays
    {
    vtkDataArray **data=new vtkDataArray * [num];
    arrayNames = new char *[num];
    // copy the original data
    for ( i=0; i < this->NumberOfArrays; i++ )
      {
      data[i] = this->Data[i];
      if ( this->ArrayNames[i] != NULL )
        {
        arrayNames[i] = new char[strlen(this->ArrayNames[i])+1];
        strcpy(arrayNames[i],this->ArrayNames[i]);
        }
      else
        {
        arrayNames[i] = NULL;
        }
      }

    //initialize the new arrays
    for ( i=this->NumberOfArrays; i < num; i++ )
      {
      data[i] = NULL;
      arrayNames[i] = NULL;
      }

    // get rid of the old data
    if ( this->Data )
      {
      delete [] this->Data;
      }
    if ( this->ArrayNames )
      {
      for ( i=0; i<this->NumberOfArrays; i++ )
        {
        if ( this->ArrayNames[i] != NULL )
	  {
	  delete [] this->ArrayNames[i];
	  }
        }
      delete [] this->ArrayNames;
      }
    
    // update object
    this->Data = data;
    this->ArrayNames = arrayNames;
    this->NumberOfArrays = num;
    }
}

// Set an array to define the field.
void vtkFieldData::SetArray(int i, vtkDataArray *data)
{
  int numComp;

  if ( i < 0 )
    {
    i = 0;
    }
  else if (i >= this->NumberOfArrays)
    {
    this->SetNumberOfArrays(i+1);
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
  return this->NumberOfArrays;
}

// Return the ith array in the field. A NULL is returned if the index i is out
// if range.
vtkDataArray *vtkFieldData::GetArray(int i)
{
  if ( i < 0 || i >= this->NumberOfArrays )
    {
    return NULL;
    }
  return this->Data[i];
}

// Get the number of components in the field. This is determined by adding
// up the components in each non-NULL array.
int vtkFieldData::GetNumberOfComponents()
{
  int i, numComp;

  for ( i=numComp=0; i < this->NumberOfArrays; i++ )
    {
    if ( this->Data[i] != NULL )
      {
      numComp += this->Data[i]->GetNumberOfComponents();
      }
    }

  return numComp;
}

// Get the number of tuples in the field.
int vtkFieldData::GetNumberOfTuples()
{
  int i, numTuples = 0;

  for ( i=0; i < this->NumberOfArrays; i++ )
    {
    if ( this->Data[i] != NULL )
      {
      numTuples = this->Data[i]->GetNumberOfTuples();
      break;
      }
    }

  return numTuples;
}

// Set the number of tuples for each data array in the field.
void vtkFieldData::SetNumberOfTuples(const int number)
{
  for ( int i=0; i < this->NumberOfArrays; i++ )
    {
    if ( this->Data[i] != NULL )
      {
      this->Data[i]->SetNumberOfTuples(number);
      }
    }
}

// Return a tuple consisting of a concatentation of all data from all
// the different arrays. Note that everything is converted to and from
// float values.
float *vtkFieldData::GetTuple(const int i)
{
  int count=0;

  for ( int j=0; j < this->NumberOfArrays; j++ )
    {
    if ( this->Data[j] != NULL )
      {
      this->Data[j]->GetTuple(i, this->Tuple + count);
      count += this->Data[j]->GetNumberOfComponents();
      }
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

  for ( int j=0; j < this->NumberOfArrays; j++ )
    {
    if ( this->Data[j] != NULL )
      {
      this->Data[j]->SetTuple(i, tuple + count);
      count += this->Data[j]->GetNumberOfComponents();
      }
    }
}

// Insert the tuple value at the ith location. Range checking is
// performed and memory allocates as necessary.
void vtkFieldData::InsertTuple(const int i, const float * tuple)
{
  int count=0;

  for ( int j=0; j < this->NumberOfArrays; j++ )
    {
    if ( this->Data[j] != NULL )
      {
      this->Data[j]->InsertTuple(i, tuple + count);
      count += this->Data[j]->GetNumberOfComponents();
      }
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

// Copy a field by creating new data arrays (i.e., duplicate storage).
void vtkFieldData::DeepCopy(vtkFieldData *f)
{
  vtkDataArray *data, *newData;

  this->SetNumberOfArrays(f->GetNumberOfArrays());

  for ( int i=0; i < this->NumberOfArrays; i++ )
    {
    if ( (data = f->GetArray(i)) != NULL )
      {
      newData = data->MakeObject(); //instantiate same type of object
      newData->DeepCopy(data);
      this->SetArray(i, newData);
      newData->Delete();
      this->SetArrayName(i, f->GetArrayName(i));
      }
    }
}

// Copy a field by reference counting the data arrays.
void vtkFieldData::ShallowCopy(vtkFieldData *f)
{
  this->SetNumberOfArrays(f->GetNumberOfArrays());

  for ( int i=0; i < this->NumberOfArrays; i++ )
    {
    this->SetArray(i, f->GetArray(i));
    this->SetArrayName(i, f->GetArrayName(i));
    }
}


// Squeezes each data array in the field (Squeeze() reclaims unused memory.)
void vtkFieldData::Squeeze()
{
  for ( int i=0; i < this->NumberOfArrays; i++ )
    {
    if ( this->Data[i] != NULL )
      {
      this->Data[i]->Squeeze();
      }
    }
}

// Resets each data array in the field (Reset() does not release memory but
// it makes the arrays look like they are empty.)
void vtkFieldData::Reset()
{
  int i;

  for ( i=0; i < this->NumberOfArrays; i++ )
    {
    if ( this->Data[i] != NULL )
      {
      this->Data[i]->Reset();
      }
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

void vtkFieldData::SetArrayName(int i, const char *name)
{
  if ( i < 0 )
    {
    i = 0;
    }
  else if (i >= this->NumberOfArrays)
    {
    this->SetNumberOfArrays(i+1);
    }

  if (this->ArrayNames == NULL)
    {
    this->ArrayNames = new char *[this->NumberOfArrays+1];
    for (int j=0; j<this->NumberOfArrays; j++)
      {
      this->ArrayNames[j] = NULL;
      }
    }

  if ( this->ArrayNames[i] != NULL )
    {
    delete [] this->ArrayNames[i];
    }

  if (name != NULL)
    {
    this->ArrayNames[i] = new char[strlen(name)+1];
    strcpy(this->ArrayNames[i],name);
    }
  else
    {
    this->ArrayNames[i] = NULL;
    }
}

char *vtkFieldData::GetArrayName(int i)
{
  static char name[256];

  if ( this->ArrayNames == NULL || this->ArrayNames[i] == NULL )
    {
    sprintf(name, "%s_%d", "Array", i);
    return name;
    }
  else
    {
    return this->ArrayNames[i];
    }
}

// Return the array containing the ith component of the field. The return value
// is an integer number n 0<=n<this->NumberOfArrays. Also, an integer value is
// returned indicating the component in the array is returned. Method returns
// -1 if specified component is not in field.
int vtkFieldData::GetArrayContainingComponent(int i, int& arrayComp)
{
  int numComp, count=0;

  for ( int j=0; j < this->NumberOfArrays; j++ )
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
  char *name;
  index = -1;
  for (i=0; i < this->NumberOfArrays; i++)
    {
    name = this->GetArrayName(i);
    if ( name && arrayName && !strcmp(name,arrayName) )
      {
      index = i;
      return this->GetArray(i);
      }
    }
  return NULL;
}

int vtkFieldData::AddArray(vtkDataArray *array)
{
  int n = this->NumberOfArrays;
  this->SetArray(n,array);
  return n;
}

int vtkFieldData::AddArray(vtkDataArray *array, const char *name)
{
  int n = this->AddArray(array);
  this->SetArrayName(n,name);
  return n;
}

int vtkFieldData::AddReplaceArray(vtkDataArray *array, const char *name)
{
    int index;

    if (index==-1)
    {
      index = this->AddArray(array, name);
    }
    else
    {
      this->SetArray(index, array);
      this->SetArrayName(index, name);
    }
    return index;
}

int vtkFieldData::AddNoReplaceArray(vtkDataArray *array, const char *name)
{
  // check an array with this name isn't already present
  if (this->GetArray(name))
    {
    vtkDebugMacro(<<"Array named " << name << " already exists, AddArray aborted");
    return -1;
    }
  return this->AddArray(array,name);
}

unsigned long vtkFieldData::GetActualMemorySize()
{
  unsigned long size=0;

  for ( int i=0; i < this->NumberOfArrays; i++ )
    {
    if ( this->Data[i] != NULL )
      {
      size += this->Data[i]->GetActualMemorySize();
      }
    }

  return size;
}

void vtkFieldData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Number Of Arrays: " << this->GetNumberOfArrays() << "\n";
  for (int i=0; i<this->NumberOfArrays; i++)
    {
    os << indent << "Array " << i << " name = " << this->GetArrayName(i) << "\n";
    }
  os << indent << "Number Of Components: " << this->GetNumberOfComponents() << "\n";
  os << indent << "Number Of Tuples: " << this->GetNumberOfTuples() << "\n";
}

