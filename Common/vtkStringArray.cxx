/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringArray.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/



// We do not provide a definition for the copy constructor or
// operator=.  Block the warning.
#ifdef _MSC_VER
# pragma warning (disable: 4661)
#endif

#include "vtkStdString.h"

#include "vtkArrayIteratorTemplate.txx"
VTK_ARRAY_ITERATOR_TEMPLATE_INSTANTIATE(vtkStdString);

#include "vtkStringArray.h"

#include "vtkArrayIteratorTemplate.h"
#include "vtkCharArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkSortDataArray.h"

#include <vtkstd/utility>
#include <vtkstd/algorithm>
#include <vtkstd/map>

// Map containing updates to a vtkStringArray that have occurred
// since we last build the vtkStringArrayLookup.
typedef vtkstd::multimap<vtkStdString, vtkIdType> vtkStringCachedUpdates;

//-----------------------------------------------------------------------------
class vtkStringArrayLookup
{
public:
  vtkStringArrayLookup() : Rebuild(true)
    {
    this->SortedArray = NULL;
    this->IndexArray = NULL;
    }
  ~vtkStringArrayLookup()
    {
    if (this->SortedArray)
      {
      this->SortedArray->Delete();
      this->SortedArray = NULL;
      }
    if (this->IndexArray)
      {
      this->IndexArray->Delete();
      this->IndexArray = NULL;
      }
    }
  vtkStringArray* SortedArray;
  vtkIdList* IndexArray;
  vtkStringCachedUpdates CachedUpdates;
  bool Rebuild;
};

vtkStandardNewMacro(vtkStringArray);

//-----------------------------------------------------------------------------

vtkStringArray::vtkStringArray(vtkIdType numComp) :
  vtkAbstractArray( numComp )
{
  this->Array = NULL;
  this->SaveUserArray = 0;
  this->Lookup = NULL;
}

//-----------------------------------------------------------------------------

vtkStringArray::~vtkStringArray()
{
  if ((this->Array) && (!this->SaveUserArray))
    {
    delete [] this->Array;
    }
  if (this->Lookup)
    {
    delete this->Lookup;
    }
}

//-----------------------------------------------------------------------------
vtkArrayIterator* vtkStringArray::NewIterator()
{
  vtkArrayIteratorTemplate<vtkStdString>* iter = 
    vtkArrayIteratorTemplate<vtkStdString>::New();
  iter->Initialize(this);
  return iter;
}

//-----------------------------------------------------------------------------
// This method lets the user specify data to be held by the array.  The
// array argument is a pointer to the data.  size is the size of
// the array supplied by the user.  Set save to 1 to keep the class
// from deleting the array when it cleans up or reallocates memory.
// The class uses the actual array provided; it does not copy the data
// from the suppled array.

void vtkStringArray::SetArray(vtkStdString *array, vtkIdType size, int save)
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
  this->DataChanged();
}

//-----------------------------------------------------------------------------
// Allocate memory for this array. Delete old storage only if necessary.

int vtkStringArray::Allocate(vtkIdType sz, vtkIdType)
{
  if(sz > this->Size)
    {
    if(this->Array && !this->SaveUserArray)
      {
      delete [] this->Array;
      }

    this->Size = ( sz > 0 ? sz : 1);
    this->Array = new vtkStdString[this->Size];
    if(!this->Array)
      {
      return 0;
      }
    this->SaveUserArray = 0;
    }

  this->MaxId = -1;
  this->DataChanged();

  return 1;
}

//-----------------------------------------------------------------------------
// Release storage and reset array to initial state.

void vtkStringArray::Initialize()
{
  if(this->Array && !this->SaveUserArray)
    {
    delete [] this->Array;
    }
  this->Array = 0;
  this->Size = 0;
  this->MaxId = -1;
  this->SaveUserArray = 0;
  this->DataChanged();
}

//-----------------------------------------------------------------------------
// Deep copy of another string array.

void vtkStringArray::DeepCopy(vtkAbstractArray* aa)
{
  // Do nothing on a NULL input.
  if(!aa)
    {
    return;
    }

  // Avoid self-copy.
  if(this == aa)
    {
    return;
    }

  // If data type does not match, we can't copy. 
  if(aa->GetDataType() != this->GetDataType())
    {
    vtkErrorMacro(<< "Incompatible types: tried to copy an array of type "
                  << aa->GetDataTypeAsString()
                  << " into a string array ");
    return;
    }

  vtkStringArray *fa = vtkStringArray::SafeDownCast( aa );
  if ( fa == NULL )
    {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkStringArray." );
    return;
    }

  // Free our previous memory.
  if(this->Array && !this->SaveUserArray)
    {
    delete [] this->Array;
    }

  // Copy the given array into new memory.
  this->MaxId = fa->GetMaxId();
  this->Size = fa->GetSize();
  this->SaveUserArray = 0;
  this->Array = new vtkStdString[this->Size];

  for (int i = 0; i < this->Size; ++i)
    {
    this->Array[i] = fa->Array[i];
    }
  this->DataChanged();
}

//-----------------------------------------------------------------------------
// Interpolate array value from other array value given the
// indices and associated interpolation weights.
// This method assumes that the two arrays are of the same time.
void vtkStringArray::InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
    vtkAbstractArray* source,  double* weights)
{
  if (this->GetDataType() != source->GetDataType())
    {
    vtkErrorMacro("Cannot CopyValue from array of type " 
      << source->GetDataTypeAsString());
    return;
    }

  if (ptIndices->GetNumberOfIds() == 0)
    {
    // nothing to do.
    return;
    }

  // We use nearest neighbour for interpolating strings.
  // First determine which is the nearest neighbour using the weights-
  // it's the index with maximum weight.
  vtkIdType nearest = ptIndices->GetId(0);
  double max_weight = weights[0];
  for (int k=1; k < ptIndices->GetNumberOfIds(); k++)
    {
    if (weights[k] > max_weight)
      {
      nearest = ptIndices->GetId(k);
      max_weight = weights[k];
      }
    }

  this->InsertTuple(i, nearest, source);
}

//-----------------------------------------------------------------------------
// Interpolate value from the two values, p1 and p2, and an 
// interpolation factor, t. The interpolation factor ranges from (0,1), 
// with t=0 located at p1. This method assumes that the three arrays are of 
// the same type. p1 is value at index id1 in fromArray1, while, p2 is
// value at index id2 in fromArray2.
void vtkStringArray::InterpolateTuple(vtkIdType i, vtkIdType id1, 
  vtkAbstractArray* source1, vtkIdType id2, vtkAbstractArray* source2, 
  double t)
{
  if (source1->GetDataType() != VTK_STRING || 
    source2->GetDataType() != VTK_STRING)
    {
    vtkErrorMacro("All arrays to InterpolateValue() must be of same type.");
    return;
    }

  if (t >= 0.5)
    {
    // Use p2
    this->InsertTuple(i, id2, source2);
    }
  else
    {
    // Use p1.
    this->InsertTuple(i, id1, source1); 
    }
}

//-----------------------------------------------------------------------------
void vtkStringArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if(this->Array)
    {
    os << indent << "Array: " << this->Array << "\n";
    }
  else
    {
    os << indent << "Array: (null)\n";
    }
}

//-----------------------------------------------------------------------------
// Protected function does "reallocate"

vtkStdString * vtkStringArray::ResizeAndExtend(vtkIdType sz)
{
  vtkStdString * newArray;
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

  newArray = new vtkStdString[newSize];
  if(!newArray)
    {
    vtkErrorMacro("Cannot allocate memory\n");
    return 0;
    }

  if(this->Array)
    {
    // can't use memcpy here
    vtkIdType numCopy = (newSize < this->Size ? newSize : this->Size);
    for (vtkIdType i = 0; i < numCopy; ++i)
      {
      newArray[i] = this->Array[i];
      }
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

  this->DataChanged();
  return this->Array;
}

//-----------------------------------------------------------------------------
int vtkStringArray::Resize(vtkIdType sz)
{
  vtkStdString * newArray;
  vtkIdType newSize = sz;

  if(newSize == this->Size)
    {
    return 1;
    }

  if(newSize <= 0)
    {
    this->Initialize();
    return 1;
    }

  newArray = new vtkStdString[newSize];
  if(!newArray)
    {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  if(this->Array)
    {
    vtkIdType numCopy = (newSize < this->Size ? newSize : this->Size);

    for (vtkIdType i = 0; i < numCopy; ++i)
      {
      newArray[i] = this->Array[i];
      }
    
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
  this->DataChanged();
  return 1;
}


//-----------------------------------------------------------------------------
void vtkStringArray::SetNumberOfValues(vtkIdType number)
{
  this->Allocate(number);
  this->MaxId = number - 1;
  this->DataChanged();
}

//-----------------------------------------------------------------------------
vtkStdString * vtkStringArray::WritePointer(vtkIdType id,
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
  this->DataChanged();
  return this->Array + id;
}

//-----------------------------------------------------------------------------
void vtkStringArray::InsertValue(vtkIdType id, vtkStdString f)
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
  this->DataElementChanged(id);
}

//-----------------------------------------------------------------------------
vtkIdType vtkStringArray::InsertNextValue(vtkStdString f)
{
  this->InsertValue (++this->MaxId,f);
  this->DataElementChanged(this->MaxId);
  return this->MaxId;
}

// ----------------------------------------------------------------------------
int vtkStringArray::GetDataTypeSize( void )
{
  return static_cast<int>(sizeof(vtkStdString));
}

// ----------------------------------------------------------------------------
unsigned long vtkStringArray::GetActualMemorySize( void )
{
  size_t totalSize = 0;
  size_t  numPrims = static_cast<size_t>(this->GetSize());

  for (size_t i = 0; i < numPrims; ++i)
    {
    totalSize += sizeof( vtkStdString );
    totalSize += this->Array[i].size() *sizeof( vtkStdString::value_type );
    }

  return static_cast<unsigned long>(
    ceil(static_cast<double>(totalSize) / 1024.0 )); // kilobytes
}

// ----------------------------------------------------------------------------
vtkIdType vtkStringArray::GetDataSize()
{
  size_t size = 0;
  size_t numStrs = static_cast<size_t>(this->GetMaxId() + 1);
  for(size_t i=0; i < numStrs; i++)
    {
    size += this->Array[i].size() + 1;
    // (+1) for termination character.
    }
  return static_cast<vtkIdType>(size);
}

// ----------------------------------------------------------------------------
// Set the tuple at the ith location using the jth tuple in the source array.
// This method assumes that the two arrays have the same type
// and structure. Note that range checking and memory allocation is not 
// performed; use in conjunction with SetNumberOfTuples() to allocate space.
void vtkStringArray::SetTuple(vtkIdType i, vtkIdType j, 
  vtkAbstractArray* source)
{
  vtkStringArray* sa = vtkStringArray::SafeDownCast(source);
  if (!sa)
    {
    vtkWarningMacro("Input and outputs array data types do not match.");
    return ;
    }

  vtkIdType loci = i * this->NumberOfComponents;
  vtkIdType locj = j * sa->GetNumberOfComponents();
  for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
    {
    this->SetValue(loci + cur, sa->GetValue(locj + cur));
    }
  this->DataChanged();
}

// ----------------------------------------------------------------------------
// Insert the jth tuple in the source array, at ith location in this array. 
// Note that memory allocation is performed as necessary to hold the data.
void vtkStringArray::InsertTuple(vtkIdType i, vtkIdType j, 
  vtkAbstractArray* source)
{
  vtkStringArray* sa = vtkStringArray::SafeDownCast(source);
  if (!sa)
    {
    vtkWarningMacro("Input and outputs array data types do not match.");
    return ;
    }

  vtkIdType loci = i * this->NumberOfComponents;
  vtkIdType locj = j * sa->GetNumberOfComponents();
  for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
    {
    this->InsertValue(loci + cur, sa->GetValue(locj + cur));
    }
  this->DataChanged();
}

// ----------------------------------------------------------------------------
// Insert the jth tuple in the source array, at the end in this array. 
// Note that memory allocation is performed as necessary to hold the data.
// Returns the location at which the data was inserted.
vtkIdType vtkStringArray::InsertNextTuple(vtkIdType j,
                                          vtkAbstractArray* source)
{
  vtkStringArray* sa = vtkStringArray::SafeDownCast(source);
  if (!sa)
    {
    vtkWarningMacro("Input and outputs array data types do not match.");
    return -1;
    }

  vtkIdType locj = j * sa->GetNumberOfComponents();
  for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
    {
    this->InsertNextValue(sa->GetValue(locj + cur));
    }
  this->DataChanged();
  return (this->GetNumberOfTuples()-1);
}

// ----------------------------------------------------------------------------
vtkStdString& vtkStringArray::GetValue( vtkIdType id )
{
  return this->Array[id];
}

// ----------------------------------------------------------------------------
void vtkStringArray::GetTuples(vtkIdList *indices, vtkAbstractArray *aa)
{
  if (aa == NULL)
    {
    vtkErrorMacro(<<"GetTuples: Output array is null!");
    return;
    }

  vtkStringArray *output = vtkStringArray::SafeDownCast(aa);

  if (output == NULL)
    {
    vtkErrorMacro(<< "Can't copy values from a string array into an array "
                  << "of type " << aa->GetDataTypeAsString());
    return;
    }

  for (vtkIdType i = 0; i < indices->GetNumberOfIds(); ++i)
    {
    vtkIdType index = indices->GetId(i);
    output->SetValue(i, this->GetValue(index));
    }
}

// ----------------------------------------------------------------------------
void vtkStringArray::GetTuples(vtkIdType startIndex,
                          vtkIdType endIndex, 
                          vtkAbstractArray *aa)
{
  if (aa == NULL)
    {
    vtkErrorMacro(<<"GetTuples: Output array is null!");
    return;
    }

  vtkStringArray *output = vtkStringArray::SafeDownCast(aa);

  if (output == NULL)
    {
    vtkErrorMacro(<< "Can't copy values from a string array into an array "
                  << "of type " << aa->GetDataTypeAsString());
    return;
    }

  for (vtkIdType i = 0; i < (endIndex - startIndex) + 1; ++i)
    {
    vtkIdType index = startIndex + i;
    output->SetValue(i, this->GetValue(index));
    }
}

//-----------------------------------------------------------------------------
void vtkStringArray::UpdateLookup()
{
  if (!this->Lookup)
    {
    this->Lookup = new vtkStringArrayLookup();
    this->Lookup->SortedArray = vtkStringArray::New();
    this->Lookup->IndexArray = vtkIdList::New();
    }
  if (this->Lookup->Rebuild)
    {
    int numComps = this->GetNumberOfComponents();
    vtkIdType numTuples = this->GetNumberOfTuples();
    this->Lookup->SortedArray->DeepCopy(this);
    this->Lookup->IndexArray->SetNumberOfIds(numComps*numTuples);
    for (vtkIdType i = 0; i < numComps*numTuples; i++)
      {
      this->Lookup->IndexArray->SetId(i, i);
      }
    vtkSortDataArray::Sort(this->Lookup->SortedArray,
                           this->Lookup->IndexArray);
    this->Lookup->Rebuild = false;
    this->Lookup->CachedUpdates.clear();
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkStringArray::LookupValue(vtkVariant var)
{
  return this->LookupValue(var.ToString());
}

//-----------------------------------------------------------------------------
void vtkStringArray::LookupValue(vtkVariant var, vtkIdList* ids)
{
  this->LookupValue(var.ToString(), ids);
}

//-----------------------------------------------------------------------------
vtkIdType vtkStringArray::LookupValue(vtkStdString value)
{
  this->UpdateLookup();

  // First look into the cached updates, to see if there were any
  // cached changes. Find an equivalent element in the set of cached
  // indices for this value. Some of the indices may have changed
  // values since the cache was built, so we need to do this equality
  // check.
  typedef vtkStringCachedUpdates::iterator CacheIterator;
  CacheIterator cached    = this->Lookup->CachedUpdates.lower_bound(value),
                cachedEnd = this->Lookup->CachedUpdates.end();
  while (cached != cachedEnd)
    {
    // Check that we are still in the same equivalence class as the
    // value.
    if (value == cached->first)
      {
      // Check that the value in the original array hasn't changed.
      vtkStdString currentValue = this->GetValue(cached->second);
      if (value == currentValue)
        {
        return cached->second;
        }
      }
    else
      {
      break;
      }

    ++cached;
    }

  int numComps = this->Lookup->SortedArray->GetNumberOfComponents();
  vtkIdType numTuples = this->Lookup->SortedArray->GetNumberOfTuples();
  vtkStdString* ptr = this->Lookup->SortedArray->GetPointer(0);
  vtkStdString* ptrEnd = ptr + numComps*numTuples;
  vtkStdString* found = vtkstd::lower_bound(ptr, ptrEnd, value);

  // Find an index with a matching value. Non-matching values might
  // show up here when the underlying value at that index has been
  // changed (so the sorted array is out-of-date).
  vtkIdType offset = static_cast<vtkIdType>(found - ptr);
  while (found != ptrEnd)
    {
    // Check whether we still have a value equivalent to what we're
    // looking for.
      if (value == *found)
      {
      // Check that the value in the original array hasn't changed.
      vtkIdType index = this->Lookup->IndexArray->GetId(offset);
      vtkStdString currentValue = this->GetValue(index);
      if (value == currentValue)
        {
        return index;
        }
      }
    else
      {
      break;
      }

    ++found; 
    ++offset;
    }

  return -1;
}

//-----------------------------------------------------------------------------
void vtkStringArray::LookupValue(vtkStdString value, vtkIdList* ids)
{
  this->UpdateLookup();
  ids->Reset();

  // First look into the cached updates, to see if there were any
  // cached changes. Find an equivalent element in the set of cached
  // indices for this value. Some of the indices may have changed
  // values since the cache was built, so we need to do this equality
  // check.
  typedef vtkStringCachedUpdates::iterator CacheIterator;
  vtkstd::pair<CacheIterator, CacheIterator> cached    
    = this->Lookup->CachedUpdates.equal_range(value);
  while (cached.first != cached.second) 
    {
    // Check that the value in the original array hasn't changed.
    vtkStdString currentValue = this->GetValue(cached.first->second);
    if (cached.first->first == currentValue)
      {
      ids->InsertNextId(cached.first->second);
      }

    ++cached.first;
    }

  // Perform a binary search of the sorted array using STL equal_range.
  int numComps = this->GetNumberOfComponents();
  vtkIdType numTuples = this->GetNumberOfTuples();
  vtkStdString* ptr = this->Lookup->SortedArray->GetPointer(0);
  vtkstd::pair<vtkStdString*,vtkStdString*> found = 
    vtkstd::equal_range(ptr, ptr + numComps*numTuples, value);

  // Add the indices of the found items to the ID list.
  vtkIdType offset = static_cast<vtkIdType>(found.first - ptr);
  while (found.first != found.second)
    {
    // Check that the value in the original array hasn't changed.
    vtkIdType index = this->Lookup->IndexArray->GetId(offset); 
    vtkStdString currentValue = this->GetValue(index);
    if (*found.first == currentValue)
      {
      ids->InsertNextId(index);
      }

    ++found.first;
    ++offset;
    }
}

//-----------------------------------------------------------------------------
void vtkStringArray::DataChanged()
{
  if (this->Lookup)
    {
    this->Lookup->Rebuild = true;
    }
}

//----------------------------------------------------------------------------
void vtkStringArray::DataElementChanged(vtkIdType id)
{
  if (this->Lookup)
    {
      if (this->Lookup->Rebuild)
        {
        // We're already going to rebuild the lookup table. Do nothing.
        return;
        }

      if (this->Lookup->CachedUpdates.size() >
          static_cast<size_t>(this->GetNumberOfTuples()/10))
        {
        // At this point, just rebuild the full table.
        this->Lookup->Rebuild = true;
        }
      else
        {
        // Insert this change into the set of cached updates
        vtkstd::pair<const vtkStdString, vtkIdType> 
          value(this->GetValue(id), id);
        this->Lookup->CachedUpdates.insert(value);
        }
    }
}

//-----------------------------------------------------------------------------
void vtkStringArray::ClearLookup()
{
  if (this->Lookup)
    {
    delete this->Lookup;
    this->Lookup = NULL;
    }
}


// ----------------------------------------------------------------------------

// 
//
// Below here are interface methods to allow values to be inserted as
// const char * instead of vtkStdString.  Yes, they're trivial.  The
// wrapper code needs them.
//
//


void vtkStringArray::SetValue( vtkIdType id, const char *value )
{
  this->SetValue( id, vtkStdString(value) );
}

void vtkStringArray::InsertValue( vtkIdType id, const char *value )
{
  this->InsertValue( id, vtkStdString( value ) );
}

void vtkStringArray::SetVariantValue( vtkIdType id, vtkVariant value )
{
  this->SetValue( id, value.ToString() );
}

vtkIdType vtkStringArray::InsertNextValue( const char *value )
{
  return this->InsertNextValue( vtkStdString( value ) );
}

vtkIdType vtkStringArray::LookupValue( const char *value )
{
  return this->LookupValue( vtkStdString( value ) );
}

void vtkStringArray::LookupValue( const char *value, vtkIdList* ids)
{
  this->LookupValue( vtkStdString( value ), ids);
}

// ----------------------------------------------------------------------------

