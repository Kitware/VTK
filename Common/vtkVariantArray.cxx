/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVariantArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// We do not provide a definition for the copy constructor or
// operator=.  Block the warning.
#ifdef _MSC_VER
# pragma warning (disable: 4661)
#endif

#include "vtkVariantArray.h"

#include "vtkDataArray.h"
#include "vtkIdList.h"
#include "vtkSortDataArray.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"

#include "vtkArrayIteratorTemplate.txx"
VTK_ARRAY_ITERATOR_TEMPLATE_INSTANTIATE(vtkVariant);

#include <utility>
#include <algorithm>
#include <map>

// Map containing updates to a vtkVariantArray that have occurred
// since we last build the vtkVariantArrayLookup.
typedef std::multimap<vtkVariant, vtkIdType, vtkVariantLessThan>
  vtkVariantCachedUpdates;

//----------------------------------------------------------------------------
class vtkVariantArrayLookup
{
public:
  vtkVariantArrayLookup() : Rebuild(true)
    {
    this->SortedArray = NULL;
    this->IndexArray = NULL;
    }
  ~vtkVariantArrayLookup()
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
  vtkVariantArray* SortedArray;
  vtkIdList* IndexArray;
  vtkVariantCachedUpdates CachedUpdates;
  bool Rebuild;
};

// 
// Standard functions
//

vtkStandardNewMacro(vtkVariantArray);
//----------------------------------------------------------------------------
void vtkVariantArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if(this->Array)
    {
    os << indent << "Array: " << this->Array << "\n";
    }
  else
    {
    os << indent << "Array: (null)\n";
    }
}

//----------------------------------------------------------------------------
vtkVariantArray::vtkVariantArray(vtkIdType numComp) :
  vtkAbstractArray( numComp )
{
  this->Array = NULL;
  this->SaveUserArray = 0;
  this->Lookup = NULL;
}

//----------------------------------------------------------------------------
vtkVariantArray::~vtkVariantArray()
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

//
// 
// Functions required by vtkAbstractArray
//
//

//----------------------------------------------------------------------------
int vtkVariantArray::Allocate(vtkIdType sz, vtkIdType)
{
  if(sz > this->Size)
    {
    if(this->Array && !this->SaveUserArray)
      {
      delete [] this->Array;
      }

    this->Size = (sz > 0 ? sz : 1);
    this->Array = new vtkVariant[this->Size];
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

//----------------------------------------------------------------------------
void vtkVariantArray::Initialize()
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

//----------------------------------------------------------------------------
int vtkVariantArray::GetDataType()
{
  return VTK_VARIANT;
}

//----------------------------------------------------------------------------
int vtkVariantArray::GetDataTypeSize()
{
  return static_cast<int>(sizeof(vtkVariant));
}

//----------------------------------------------------------------------------
int vtkVariantArray::GetElementComponentSize()
{
  return this->GetDataTypeSize();
}

//----------------------------------------------------------------------------
void vtkVariantArray::SetNumberOfTuples(vtkIdType number)
{
  this->SetNumberOfValues(this->NumberOfComponents * number);
  this->DataChanged();
}

//----------------------------------------------------------------------------
void vtkVariantArray::SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source)
{
  if (source->IsA("vtkVariantArray"))
    {
    vtkVariantArray* a = vtkVariantArray::SafeDownCast(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      this->SetValue(loci + cur, a->GetValue(locj + cur));
      }
    }
  else if (source->IsA("vtkDataArray"))
    {
    vtkDataArray* a = vtkDataArray::SafeDownCast(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      // TODO : This just makes a double variant by default.
      //        We really should make the appropriate type of variant
      //        based on the subclass of vtkDataArray.
      vtkIdType tuple = (locj + cur) / a->GetNumberOfComponents();
      int component = static_cast<int>((locj + cur) % a->GetNumberOfComponents());
      this->SetValue(loci + cur, vtkVariant(a->GetComponent(tuple, component)));
      }
    }
  else if (source->IsA("vtkStringArray"))
    {
    vtkStringArray* a = vtkStringArray::SafeDownCast(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      this->SetValue(loci + cur, vtkVariant(a->GetValue(locj + cur)));
      }
    }
  else
    {
    vtkWarningMacro("Unrecognized type is incompatible with vtkVariantArray.");
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
void vtkVariantArray::InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source)
{
  if (source->IsA("vtkVariantArray"))
    {
    vtkVariantArray* a = vtkVariantArray::SafeDownCast(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      this->InsertValue(loci + cur, a->GetValue(locj + cur));
      }
    }
  else if (source->IsA("vtkDataArray"))
    {
    vtkDataArray* a = vtkDataArray::SafeDownCast(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      vtkIdType tuple = (locj + cur) / a->GetNumberOfComponents();
      int component = static_cast<int>((locj + cur) % a->GetNumberOfComponents());
      this->InsertValue(loci + cur, vtkVariant(a->GetComponent(tuple, component)));
      }
    }
  else if (source->IsA("vtkStringArray"))
    {
    vtkStringArray* a = vtkStringArray::SafeDownCast(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      this->InsertValue(loci + cur, vtkVariant(a->GetValue(locj + cur)));
      }
    }
  else
    {
    vtkWarningMacro("Unrecognized type is incompatible with vtkVariantArray.");
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
vtkIdType vtkVariantArray::InsertNextTuple(vtkIdType j, vtkAbstractArray* source)
{
  if (source->IsA("vtkVariantArray"))
    {
    vtkVariantArray* a = vtkVariantArray::SafeDownCast(source);
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      this->InsertNextValue(a->GetValue(locj + cur));
      }
    }
  else if (source->IsA("vtkDataArray"))
    {
    vtkDataArray* a = vtkDataArray::SafeDownCast(source);
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      vtkIdType tuple = (locj + cur) / a->GetNumberOfComponents();
      int component = static_cast<int>((locj + cur) % a->GetNumberOfComponents());
      this->InsertNextValue(vtkVariant(a->GetComponent(tuple, component)));
      }
    }
  else if (source->IsA("vtkStringArray"))
    {
    vtkStringArray* a = vtkStringArray::SafeDownCast(source);
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      this->InsertNextValue(vtkVariant(a->GetValue(locj + cur)));
      }
    }
  else
    {
    vtkWarningMacro("Unrecognized type is incompatible with vtkVariantArray.");
    return -1;
    }

  this->DataChanged();
  return (this->GetNumberOfTuples()-1);
}

//----------------------------------------------------------------------------
void* vtkVariantArray::GetVoidPointer(vtkIdType id)
{
  return this->GetPointer(id);
}

//----------------------------------------------------------------------------
void vtkVariantArray::DeepCopy(vtkAbstractArray *aa)
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
                  << " into a variant array ");
    return;
    }

  vtkVariantArray *va = vtkVariantArray::SafeDownCast( aa );
  if ( va == NULL )
    {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkVariantArray." );
    return;
    }

  // Free our previous memory.
  if(this->Array && !this->SaveUserArray)
    {
    delete [] this->Array;
    }

  // Copy the given array into new memory.
  this->MaxId = va->GetMaxId();
  this->Size = va->GetSize();
  this->SaveUserArray = 0;
  this->Array = new vtkVariant[this->Size];

  for (int i = 0; i < this->Size; ++i)
    {
    this->Array[i] = va->Array[i];
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
void vtkVariantArray::InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
  vtkAbstractArray* source,  double* weights)
{
  // Note: Something much more fancy could be done here, allowing
  // the source array be any data type.
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

  // We use nearest neighbour for interpolating variants.
  // First determine which is the nearest neighbour using the weights-
  // it's the index with maximum weight.
  vtkIdType nearest = ptIndices->GetId(0);
  double max_weight = weights[0];
  for (int k=1; k < ptIndices->GetNumberOfIds(); k++)
    {
    if (weights[k] > max_weight)
      {
      nearest = k;
      }
    }

  this->InsertTuple(i, nearest, source);
  this->DataChanged();
}

//----------------------------------------------------------------------------
void vtkVariantArray::InterpolateTuple(vtkIdType i, 
  vtkIdType id1, vtkAbstractArray* source1, 
  vtkIdType id2, vtkAbstractArray* source2, double t)
{
  // Note: Something much more fancy could be done here, allowing
  // the source array to be any data type.
  if (source1->GetDataType() != VTK_VARIANT || 
    source2->GetDataType() != VTK_VARIANT)
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
  this->DataChanged();
}

//----------------------------------------------------------------------------
void vtkVariantArray::Squeeze()
{
  this->ResizeAndExtend(this->MaxId + 1);
}

//----------------------------------------------------------------------------
int vtkVariantArray::Resize(vtkIdType sz)
{
  vtkVariant* newArray;
  vtkIdType newSize = sz * this->GetNumberOfComponents();

  if(newSize == this->Size)
    {
    return 1;
    }

  if(newSize <= 0)
    {
    this->Initialize();
    return 1;
    }

  newArray = new vtkVariant[newSize];
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

//----------------------------------------------------------------------------
void vtkVariantArray::SetVoidArray(void *arr, vtkIdType size, int save)
{
  this->SetArray(static_cast<vtkVariant*>(arr), size, save);
  this->DataChanged();
}

//----------------------------------------------------------------------------
unsigned long vtkVariantArray::GetActualMemorySize()
{
  // NOTE: Currently does not take into account the "pointed to" data.
  size_t totalSize = 0;
  size_t numPrims = static_cast<size_t>(this->GetSize());

  totalSize = numPrims*sizeof(vtkVariant);

  return static_cast<unsigned long>(
    ceil(static_cast<double>(totalSize) / 1024.0)); // kilobytes
}

//----------------------------------------------------------------------------
int vtkVariantArray::IsNumeric()
{
  return 0;
}

//----------------------------------------------------------------------------
vtkArrayIterator* vtkVariantArray::NewIterator()
{
  vtkArrayIteratorTemplate<vtkVariant>* iter = 
    vtkArrayIteratorTemplate<vtkVariant>::New();
  iter->Initialize(this);
  return iter;
}

//
//
// Additional functions
//
//

//----------------------------------------------------------------------------
vtkVariant& vtkVariantArray::GetValue(vtkIdType id) const
{
  return this->Array[id];
}

//----------------------------------------------------------------------------
void vtkVariantArray::SetValue(vtkIdType id, vtkVariant value)
{
  this->Array[id] = value;
  this->DataElementChanged(id);
}

//----------------------------------------------------------------------------
void vtkVariantArray::InsertValue(vtkIdType id, vtkVariant value)
{
  if ( id >= this->Size )
    {
    this->ResizeAndExtend(id+1);
    }
  this->Array[id] = value;
  if ( id > this->MaxId )
    {
    this->MaxId = id;
    }
  this->DataElementChanged(id);
}

//----------------------------------------------------------------------------
void vtkVariantArray::SetVariantValue(vtkIdType id, vtkVariant value)
{
  this->SetValue(id, value);
}

//----------------------------------------------------------------------------
vtkIdType vtkVariantArray::InsertNextValue(vtkVariant value)
{
  this->InsertValue(++this->MaxId, value);
  this->DataElementChanged(this->MaxId);
  return this->MaxId;
}

//----------------------------------------------------------------------------
void vtkVariantArray::SetNumberOfValues(vtkIdType number)
{
  this->Allocate(number);
  this->MaxId = number - 1;
  this->DataChanged();
}

//----------------------------------------------------------------------------
vtkVariant* vtkVariantArray::GetPointer(vtkIdType id)
{
  return this->Array + id;
}

//----------------------------------------------------------------------------
void vtkVariantArray::SetArray(vtkVariant* arr, vtkIdType size, int save)
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

  vtkDebugMacro(<<"Setting array to: " << arr);

  this->Array = arr;
  this->Size = size;
  this->MaxId = size-1;
  this->SaveUserArray = save;
  this->DataChanged();
}

//----------------------------------------------------------------------------
vtkVariant* vtkVariantArray::ResizeAndExtend(vtkIdType sz)
{
  vtkVariant* newArray;
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

  newArray = new vtkVariant[newSize];
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

//----------------------------------------------------------------------------
void vtkVariantArray::UpdateLookup()
{
  if (!this->Lookup)
    {
    this->Lookup = new vtkVariantArrayLookup();
    this->Lookup->SortedArray = vtkVariantArray::New();
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
    vtkSortDataArray::Sort(this->Lookup->SortedArray, this->Lookup->IndexArray);
    this->Lookup->Rebuild = false;
    this->Lookup->CachedUpdates.clear();
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkVariantArray::LookupValue(vtkVariant value)
{
  this->UpdateLookup();
  
  // First look into the cached updates, to see if there were any
  // cached changes. Find an equivalent element in the set of cached
  // indices for this value. Some of the indices may have changed
  // values since the cache was built, so we need to do this equality
  // check.
  typedef vtkVariantCachedUpdates::iterator CacheIterator;
  CacheIterator cached    = this->Lookup->CachedUpdates.lower_bound(value),
                cachedEnd = this->Lookup->CachedUpdates.end();
  while (cached != cachedEnd)
    {
    // Check that we are still in the same equivalence class as the
    // value.
    if (value == (*cached).first)
      {
      // Check that the value in the original array hasn't changed.
      vtkVariant currentValue = this->GetValue(cached->second);
      if (value == currentValue)
        {
        return (*cached).second;
        }
      }
    else
      {
      break;
      }

    ++cached;
    }

  // Perform a binary search of the sorted array using STL equal_range.
  int numComps = this->Lookup->SortedArray->GetNumberOfComponents();
  vtkIdType numTuples = this->Lookup->SortedArray->GetNumberOfTuples();
  vtkVariant* ptr = this->Lookup->SortedArray->GetPointer(0);
  vtkVariant* ptrEnd = ptr + numComps*numTuples;
  vtkVariant* found = std::lower_bound(
    ptr, ptrEnd, value, vtkVariantLessThan());

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
      vtkVariant currentValue = this->GetValue(index);
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

//----------------------------------------------------------------------------
void vtkVariantArray::LookupValue(vtkVariant value, vtkIdList* ids)
{
  this->UpdateLookup();
  ids->Reset();

  // First look into the cached updates, to see if there were any
  // cached changes. Find an equivalent element in the set of cached
  // indices for this value. Some of the indices may have changed
  // values since the cache was built, so we need to do this equality
  // check.
  typedef vtkVariantCachedUpdates::iterator CacheIterator;
  std::pair<CacheIterator, CacheIterator> cached
    = this->Lookup->CachedUpdates.equal_range(value);
  while (cached.first != cached.second) 
    {
    // Check that the value in the original array hasn't changed.
    vtkVariant currentValue = this->GetValue(cached.first->second);
    if (cached.first->first == currentValue)
      {
      ids->InsertNextId(cached.first->second);
      }

    ++cached.first;
    }
  
  // Perform a binary search of the sorted array using STL equal_range.
  int numComps = this->GetNumberOfComponents();
  vtkIdType numTuples = this->GetNumberOfTuples();
  vtkVariant* ptr = this->Lookup->SortedArray->GetPointer(0);
  vtkVariant* ptrEnd = ptr + numComps*numTuples;
  std::pair<vtkVariant*, vtkVariant*> found =
    std::equal_range(ptr, ptrEnd, value, vtkVariantLessThan());
  
  // Add the indices of the found items to the ID list.
  vtkIdType offset = static_cast<vtkIdType>(found.first - ptr);
  while (found.first != found.second)
    {
    // Check that the value in the original array hasn't changed.
    vtkIdType index = this->Lookup->IndexArray->GetId(offset); 
    vtkVariant currentValue = this->GetValue(index);
    if (*(found.first) == currentValue)
      {
      ids->InsertNextId(index);
      }

    ++found.first;
    ++offset;
    }
}

//----------------------------------------------------------------------------
void vtkVariantArray::DataChanged()
{
  if (this->Lookup)
    {
    this->Lookup->Rebuild = true;
    }
}

//----------------------------------------------------------------------------
void vtkVariantArray::DataElementChanged(vtkIdType id)
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
        std::pair<const vtkVariant, vtkIdType>
          value(this->GetValue(id), id);
        this->Lookup->CachedUpdates.insert(value);
        }
    }
}

//----------------------------------------------------------------------------
void vtkVariantArray::ClearLookup()
{
  if (this->Lookup)
    {
    delete this->Lookup;
    this->Lookup = NULL;
    }
}
