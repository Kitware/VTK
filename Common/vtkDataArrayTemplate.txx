/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayTemplate.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkDataArrayTemplate_txx
#define __vtkDataArrayTemplate_txx

#include "vtkDataArrayTemplate.h"

#include <vtkstd/exception>

// We do not provide a definition for the copy constructor or
// operator=.  Block the warning.
#ifdef _MSC_VER
# pragma warning (disable: 4661)
#endif

//----------------------------------------------------------------------------
template <class T>
vtkDataArrayTemplate<T>::vtkDataArrayTemplate(vtkIdType numComp):
  vtkDataArray(numComp)
{
  this->Array = 0;
  this->TupleSize = 3;
  this->Tuple = new double[this->TupleSize]; //used for conversion
  this->SaveUserArray = 0;
}

//----------------------------------------------------------------------------
template <class T>
vtkDataArrayTemplate<T>::~vtkDataArrayTemplate()
{
  if ((this->Array) && (!this->SaveUserArray))
    {
    delete [] this->Array;
    }
  delete [] this->Tuple;
}

//----------------------------------------------------------------------------
// This method lets the user specify data to be held by the array.  The
// array argument is a pointer to the data.  size is the size of
// the array supplied by the user.  Set save to 1 to keep the class
// from deleting the array when it cleans up or reallocates memory.
// The class uses the actual array provided; it does not copy the data
// from the suppled array.
template <class T>
void vtkDataArrayTemplate<T>::SetArray(T* array, vtkIdType size, int save)
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

  vtkDebugMacro(<<"Setting array to: " << static_cast<void*>(array));

  this->Array = array;
  this->Size = size;
  this->MaxId = size-1;
  this->SaveUserArray = save;
}

//----------------------------------------------------------------------------
// Allocate memory for this array. Delete old storage only if necessary.
template <class T>
int vtkDataArrayTemplate<T>::Allocate(vtkIdType sz, vtkIdType)
{
  this->MaxId = -1;

  if(sz > this->Size)
    {
    if(this->Array && !this->SaveUserArray)
      {
      delete [] this->Array;
      }

    this->Array = 0;
    this->Size = 0;
    this->SaveUserArray = 0;

    int newSize = (sz > 0 ? sz : 1);
    try
      {
      this->Array = new T[newSize];
      }
    catch (vtkstd::exception& e)
      {
      vtkErrorMacro("Unable to allocate " << newSize
                    << " elements of size " << sizeof(T)
                    << ": " << e.what());
      this->Array = 0;
      }
    if(!this->Array)
      {
      return 0;
      }
    this->Size = newSize;
    }

  return 1;
}

//----------------------------------------------------------------------------
// Release storage and reset array to initial state.
template <class T>
void vtkDataArrayTemplate<T>::Initialize()
{
  if(this->Array && !this->SaveUserArray)
    {
    delete [] this->Array;
    }
  this->Array = 0;
  this->Size = 0;
  this->MaxId = -1;
  this->SaveUserArray = 0;
}

//----------------------------------------------------------------------------
// Deep copy of another double array.
template <class T>
void vtkDataArrayTemplate<T>::DeepCopy(vtkDataArray* fa)
{
  // Do nothing on a NULL input.
  if(!fa)
    {
    return;
    }

  // Avoid self-copy.
  if(this == fa)
    {
    return;
    }

  // If data type does not match, do copy with conversion.
  if(fa->GetDataType() != this->GetDataType())
    {
    this->Superclass::DeepCopy(fa);
    return;
    }

  // Free our previous memory.
  if(this->Array && !this->SaveUserArray)
    {
    delete [] this->Array;
    }

  // Copy the given array into new memory.
  this->NumberOfComponents = fa->GetNumberOfComponents();
  this->MaxId = fa->GetMaxId();
  this->Size = fa->GetSize();
  this->SaveUserArray = 0;
  try
    {
    this->Array = new T[this->Size];
    }
  catch (vtkstd::exception& e)
    {
    vtkErrorMacro("Unable to allocate " <<  this->Size << " bytes: "
                  << e.what());
    this->Size = 0;
    this->NumberOfComponents = 0;
    this->MaxId = -1;
    this->Array = 0;
    return;
    }
  memcpy(this->Array, fa->GetVoidPointer(0), this->Size*sizeof(T));
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  vtkOStreamWrapper osw(os);
  if(this->Array)
    {
    osw << indent << "Array: " << static_cast<void*>(this->Array) << "\n";
    }
  else
    {
    osw << indent << "Array: (null)\n";
    }
}

//----------------------------------------------------------------------------
// Protected function does "reallocate"
template <class T>
T* vtkDataArrayTemplate<T>::ResizeAndExtend(vtkIdType sz)
{
  T* newArray;
  vtkIdType newSize;

  if(sz > this->Size)
    {
    // Requested size is bigger than current size.  Allocate enough
    // memory to fit the requested size and be more than double the
    // currently allocated memory.
    newSize = this->Size + sz;
    }
  else if (sz == this->Size)
    {
    // Requested size is equal to current size.  Do nothing.
    return this->Array;
    }
  else
    {
    // Requested size is smaller than current size.  Squeeze the
    // memory.
    newSize = sz;
    }

  if(newSize <= 0)
    {
    this->Initialize();
    return 0;
    }
  try
    {
    newArray = new T[newSize];
    }
  catch (vtkstd::exception& e)
    {
    vtkErrorMacro("Unable to allocate " << newSize
                  << " elements of size " << sizeof(T)
                  << ": " << e.what());
    return 0;
    }
  if(!newArray)
    {
    vtkErrorMacro("Cannot allocate memory\n");
    return 0;
    }

  if(this->Array)
    {
    memcpy(newArray, this->Array,
           (sz < this->Size ? sz : this->Size) * sizeof(T));
    if(!this->SaveUserArray)
      {
      delete [] this->Array;
      }
    }

  if(newSize < this->Size)
    {
    this->MaxId = newSize-1;
    }
  this->Size = newSize;
  this->Array = newArray;
  this->SaveUserArray = 0;

  return this->Array;
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::Resize(vtkIdType sz)
{
  T* newArray;
  vtkIdType newSize = sz*NumberOfComponents;

  if(newSize == this->Size)
    {
    return;
    }

  if(newSize <= 0)
    {
    this->Initialize();
    return;
    }

  try
    {
    newArray = new T[newSize];
    }
  catch (vtkstd::exception& e)
    {
    vtkErrorMacro("Unable to allocate " << newSize
                  << " elements of size " << sizeof(T)
                  << ": " << e.what());
    return;
    }
  if(!newArray)
    {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return;
    }

  if(this->Array)
    {
    memcpy(newArray, this->Array,
           (newSize < this->Size ? newSize : this->Size) * sizeof(T));
    if(!this->SaveUserArray)
      {
      delete[] this->Array;
      }
    }

  if(newSize < this->Size)
    {
    this->MaxId = newSize-1;
    }
  this->Size = newSize;
  this->Array = newArray;
  this->SaveUserArray = 0;
}

//----------------------------------------------------------------------------
// Set the number of n-tuples in the array.
template <class T>
void vtkDataArrayTemplate<T>::SetNumberOfTuples(vtkIdType number)
{
  this->SetNumberOfValues(number*this->NumberOfComponents);
}

//----------------------------------------------------------------------------
// Get a pointer to a tuple at the ith location. This is a dangerous method
// (it is not thread safe since a pointer is returned).
template <class T>
double* vtkDataArrayTemplate<T>::GetTuple(vtkIdType i)
{
#if 1
  if(this->TupleSize < this->NumberOfComponents)
    {
    this->TupleSize = this->NumberOfComponents;
    delete [] this->Tuple;
    this->Tuple = new double[this->TupleSize];
    }

  T* t = this->Array + this->NumberOfComponents*i;
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    this->Tuple[j] = static_cast<double>(t[j]);
    }
  return this->Tuple;
#else
  // Use this version along with purify or valgrind to detect code
  // that saves the pointer returned by GetTuple.
  T* t = this->Array + this->NumberOfComponents*i;
  double* newTuple = new double[this->NumberOfComponents];
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    newTuple[j] = static_cast<double>(t[j]);
    }
  delete [] this->Tuple;
  this->Tuple = newTuple;
  return this->Tuple;
#endif
}

//----------------------------------------------------------------------------
// Copy the tuple value into a user-provided array.
template <class T>
void vtkDataArrayTemplate<T>::GetTuple(vtkIdType i, double* tuple)
{
  T* t = this->Array + this->NumberOfComponents*i;
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    tuple[j] = static_cast<double>(t[j]);
    }
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::GetTupleValue(vtkIdType i, T* tuple)
{
  T* t = this->Array + this->NumberOfComponents*i;
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    tuple[j] = t[j];
    }
}

//----------------------------------------------------------------------------
// Set the tuple value at the ith location in the array.
template <class T>
void vtkDataArrayTemplate<T>::SetTuple(vtkIdType i, const float* tuple)
{
  vtkIdType loc = i * this->NumberOfComponents;
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    this->Array[loc+j] = static_cast<T>(tuple[j]);
    }
}

template <class T>
void vtkDataArrayTemplate<T>::SetTuple(vtkIdType i, const double* tuple)
{
  vtkIdType loc = i * this->NumberOfComponents;
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    this->Array[loc+j] = static_cast<T>(tuple[j]);
    }
}

template <class T>
void vtkDataArrayTemplate<T>::SetTupleValue(vtkIdType i, const T* tuple)
{
  vtkIdType loc = i * this->NumberOfComponents;
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    this->Array[loc+j] = tuple[j];
    }
}

//----------------------------------------------------------------------------
// Insert (memory allocation performed) the tuple into the ith location
// in the array.
template <class T>
void vtkDataArrayTemplate<T>::InsertTuple(vtkIdType i, const float* tuple)
{
  T* t = this->WritePointer(i*this->NumberOfComponents,
                            this->NumberOfComponents);

  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    *t++ = static_cast<T>(*tuple++);
    }
}

template <class T>
void vtkDataArrayTemplate<T>::InsertTuple(vtkIdType i, const double* tuple)
{
  T* t = this->WritePointer(i*this->NumberOfComponents,
                            this->NumberOfComponents);

  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    *t++ = static_cast<T>(*tuple++);
    }
}

template <class T>
void vtkDataArrayTemplate<T>::InsertTupleValue(vtkIdType i, const T* tuple)
{
  T* t = this->WritePointer(i*this->NumberOfComponents,
                            this->NumberOfComponents);

  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    *t++ = *tuple++;
    }
}

//----------------------------------------------------------------------------
// Insert (memory allocation performed) the tuple onto the end of the array.
template <class T>
vtkIdType vtkDataArrayTemplate<T>::InsertNextTuple(const float* tuple)
{
  T* t = this->WritePointer(this->MaxId + 1, this->NumberOfComponents);

  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    *t++ = static_cast<T>(*tuple++);
    }

  return this->MaxId / this->NumberOfComponents;
}

template <class T>
vtkIdType vtkDataArrayTemplate<T>::InsertNextTuple(const double* tuple)
{
  T* t = this->WritePointer(this->MaxId + 1,this->NumberOfComponents);

  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    *t++ = static_cast<T>(*tuple++);
    }

  return this->MaxId / this->NumberOfComponents;
}

template <class T>
vtkIdType vtkDataArrayTemplate<T>::InsertNextTupleValue(const T* tuple)
{
  T* t = this->WritePointer(this->MaxId + 1,this->NumberOfComponents);

  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    *t++ = *tuple++;
    }

  return this->MaxId / this->NumberOfComponents;
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::RemoveTuple(vtkIdType id)
{
  if ( id < 0 || id >= this->GetNumberOfTuples())
    {
    // Nothing to be done
    return;
    }
  if ( id == this->GetNumberOfTuples() - 1 )
    {
    // To remove last item, just decrease the size by one
    this->RemoveLastTuple();
    return;
    }
  vtkIdType len = (this->GetNumberOfTuples() - id) - 1;
  len *= this->GetNumberOfComponents();
  vtkIdType from = (id+1) * this->GetNumberOfComponents();
  vtkIdType to = id * this->GetNumberOfComponents();
  memcpy(this->Array + to, this->Array + from, len * sizeof(T));
  this->Resize(this->GetNumberOfTuples() - 1);
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::RemoveFirstTuple()
{
  this->RemoveTuple(0);
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::RemoveLastTuple()
{
  this->Resize(this->GetNumberOfTuples()- 1);
}

//----------------------------------------------------------------------------
// Return the data component at the ith tuple and jth component location.
// Note that i<NumberOfTuples and j<NumberOfComponents.
template <class T>
double vtkDataArrayTemplate<T>::GetComponent(vtkIdType i, int j)
{
  return static_cast<double>(this->GetValue(i*this->NumberOfComponents + j));
}

//----------------------------------------------------------------------------
// Set the data component at the ith tuple and jth component location.
// Note that i<NumberOfTuples and j<NumberOfComponents. Make sure enough
// memory has been allocated (use SetNumberOfTuples() and
// SetNumberOfComponents()).
template <class T>
void vtkDataArrayTemplate<T>::SetComponent(vtkIdType i, int j,
                                           double c)
{
  this->SetValue(i*this->NumberOfComponents + j, static_cast<T>(c));
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::InsertComponent(vtkIdType i, int j,
                                              double c)
{
  this->InsertValue(i*this->NumberOfComponents + j, static_cast<T>(c));
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::SetNumberOfValues(vtkIdType number)
{
  if(this->Allocate(number))
    {
    this->MaxId = number - 1;
    }
}

//----------------------------------------------------------------------------
template <class T>
T* vtkDataArrayTemplate<T>::WritePointer(vtkIdType id,
                                         vtkIdType number)
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
  return this->Array + id;
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::InsertValue(vtkIdType id, T f)
{
  if ( id >= this->Size )
    {
    this->ResizeAndExtend(id+1);
    }
  this->Array[id] = f;
  if ( id > this->MaxId )
    {
    this->MaxId = id;
    }
}

//----------------------------------------------------------------------------
template <class T>
vtkIdType vtkDataArrayTemplate<T>::InsertNextValue(T f)
{
  this->InsertValue (++this->MaxId,f);
  return this->MaxId;
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::ComputeRange(int comp)
{
  // If we got component -1 on a vector array, compute vector magnitude.
  if(comp < 0 && this->NumberOfComponents == 1)
    {
    comp = 0;
    }

  // Choose index into component range cache.
  int index = (comp<0)? this->NumberOfComponents : comp;

  if(index >= VTK_MAXIMUM_NUMBER_OF_CACHED_COMPONENT_RANGES ||
     (this->GetMTime() > this->ComponentRangeComputeTime[index]))
    {
    // We need to compute the range.
    this->Range[0] =  VTK_DOUBLE_MAX;
    this->Range[1] =  VTK_DOUBLE_MIN;

    if(comp >= 0)
      {
      this->ComputeScalarRange(comp);
      }
    else
      {
      this->ComputeVectorRange();
      }

    // Store the result in the range cache if there is room.
    if(index < VTK_MAXIMUM_NUMBER_OF_CACHED_COMPONENT_RANGES)
      {
      this->ComponentRangeComputeTime[index].Modified();
      this->ComponentRange[index][0] = this->Range[0];
      this->ComponentRange[index][1] = this->Range[1];
      }
    }
  else
    {
    // Copy value from range cache entry for this component.
    this->Range[0] = this->ComponentRange[index][0];
    this->Range[1] = this->ComponentRange[index][1];
    }
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::ComputeScalarRange(int comp)
{
  // Compute range only if there are data.
  T* begin = this->Array+comp;
  T* end = this->Array+comp+this->MaxId+1;
  if(begin == end)
    {
    return;
    }

  // Compute the range of scalar values.
  int numComp = this->NumberOfComponents;
  T range[2] = {*begin, *begin};
  for(T* i = begin+numComp; i != end; i += numComp)
    {
    T s = *i;
    if(s < range[0])
      {
      range[0] = s;
      }
    else if(s > range[1])
      {
      range[1] = s;
      }
    }

  // Store the range.
  this->Range[0] = range[0];
  this->Range[1] = range[1];
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::ComputeVectorRange()
{
  // Compute range only if there are data.
  T* begin = this->Array;
  T* end = this->Array+this->MaxId+1;
  if(begin == end)
    {
    return;
    }

  // Compute the range of vector magnitude squared.
  int numComp = this->NumberOfComponents;
  double range[2] = {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};
  for(T* i = begin; i != end; i += numComp)
    {
    double s = 0.0;
    for(int j=0; j < numComp; ++j)
      {
      double t = i[j];
      s += t*t;
      }
    if(s < range[0])
      {
      range[0] = s;
      }
    // this cannot be an elseif because there may be only one vector in which
    // case the range[1] would be left at a bad value
    if(s > range[1])
      {
      range[1] = s;
      }
    }

  // Store the range of vector magnitude.
  this->Range[0] = sqrt(range[0]);
  this->Range[1] = sqrt(range[1]);
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::ExportToVoidPointer(void *out_ptr)
{
  if(out_ptr && this->Array) 
    {
    memcpy(static_cast<T*>(out_ptr), this->Array, 
           (this->MaxId + 1)*sizeof(T));
    }
}

#endif
