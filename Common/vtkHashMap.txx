/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHashMap.txx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Include blockers needed since vtkHashMap.h includes this file
// when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.
#ifndef __vtkHashMap_txx
#define __vtkHashMap_txx

#include "vtkHashMap.h"
#include "vtkVector.txx"
#include "vtkAbstractMap.txx"
#include "vtkHashMapIterator.txx"

//----------------------------------------------------------------------------
template <class KeyType,class DataType>
vtkHashMap<KeyType,DataType> *vtkHashMap<KeyType,DataType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkHashMap");
#endif
  return new vtkHashMap<KeyType,DataType>(); 
}

//----------------------------------------------------------------------------
template <class KeyType, class DataType>
vtkHashMapIterator<KeyType,DataType>*
vtkHashMap<KeyType,DataType>::NewIterator()
{
  IteratorType* it = IteratorType::New();
  it->SetContainer(this);
  it->InitTraversal();
  return it;
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
int vtkHashMap<KeyType,DataType>::SetItem(const KeyType& key,
                                          const DataType& data)
{
  vtkIdType bucket = vtkHashMapHashMethod(key) % this->NumberOfBuckets;
  ItemType item = { key, data };
  vtkIdType index;
  
  if(this->Buckets[bucket]->FindItem(item, index) == VTK_OK)
    {
    this->Buckets[bucket]->SetItemNoCheck(index, item);
    return VTK_OK;
    }
  if(this->Buckets[bucket]->AppendItem(item))
    {
    ++this->NumberOfItems;
    this->CheckLoadFactor();
    return VTK_OK;
    }
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
int vtkHashMap<KeyType,DataType>::RemoveItem(const KeyType& key)
{
  vtkIdType bucket = vtkHashMapHashMethod(key) % this->NumberOfBuckets;
  ItemType item = { key, DataType() };
  vtkIdType index;
  
  if(this->Buckets[bucket]->FindItem(item, index) == VTK_OK)
    {
    --this->NumberOfItems;
    return this->Buckets[bucket]->RemoveItem(index);
    }
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
void vtkHashMap<KeyType,DataType>::RemoveAllItems()
{
  vtkIdType i;
  for(i=0; i < this->NumberOfBuckets; ++i)
    {
    this->Buckets[i]->RemoveAllItems();
    }
  this->NumberOfItems = 0;
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
int vtkHashMap<KeyType,DataType>::GetItem(const KeyType& key, DataType& data)
{
  vtkIdType bucket = vtkHashMapHashMethod(key) % this->NumberOfBuckets;
  ItemType item = { key, DataType() };
  vtkIdType index;
  
  if(this->Buckets[bucket]->FindItem(item, index) == VTK_OK)
    {
    this->Buckets[bucket]->GetItemNoCheck(index, item);
    data = item.Data;
    return VTK_OK;
    }
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
vtkIdType vtkHashMap<KeyType,DataType>::GetNumberOfItems() const
{
  return this->NumberOfItems;
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
void vtkHashMap<KeyType,DataType>::SetMaximumLoadFactor(float factor)
{
  // Set the load factor.
  this->MaximumLoadFactor = factor;
  if(this->MaximumLoadFactor < 0)
    {
    this->MaximumLoadFactor = 0;
    }
  
  // Re-hash if necessary.
  this->CheckLoadFactor();
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
float vtkHashMap<KeyType,DataType>::GetMaximumLoadFactor() const
{
  return this->MaximumLoadFactor;
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
void vtkHashMap<KeyType,DataType>::SetNumberOfBuckets(vtkIdType n)
{
  // Disable automatic rehashing.
  this->MaximumLoadFactor = 0;
  
  // Set the number of buckets.
  this->RehashItems((n > 0)? n : 1);
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
vtkIdType vtkHashMap<KeyType,DataType>::GetNumberOfBuckets() const
{
  return this->NumberOfBuckets;
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
void vtkHashMap<KeyType,DataType>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkIdType i;
  vtkIdType min = this->NumberOfItems;
  vtkIdType max = 0;
  for(i=0;i < this->NumberOfBuckets; ++i)
    {
    vtkIdType num = this->Buckets[i]->GetNumberOfItems();
    if(num < min) { min = num; }
    if(num > max) { max = num; }
    }
  float loadFactor = float(this->NumberOfItems) / float(this->NumberOfBuckets);
  
  os << indent << "NumberOfItems: " << this->NumberOfItems << "\n";
  os << indent << "NumberOfBuckets: " << this->NumberOfBuckets << "\n";
  os << indent << "MaximumLoadFactor: " << this->MaximumLoadFactor << "\n";
  os << indent << "Current Load Factor: " << loadFactor << "\n";
  os << indent << "Min in Bucket: " << min << "\n";
  os << indent << "Max in Bucket: " << max << "\n";
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
vtkHashMap<KeyType,DataType>::vtkHashMap() 
{
  this->NumberOfItems = 0;
  this->NumberOfBuckets = 17;
  this->Buckets = new BucketType*[this->NumberOfBuckets];
  vtkIdType i;
  for(i=0; i < this->NumberOfBuckets; ++i)
    {
    this->Buckets[i] = BucketType::New();
    }
  this->MaximumLoadFactor = 2.0;
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
vtkHashMap<KeyType,DataType>::~vtkHashMap() 
{
  vtkIdType i;
  for(i=0; i < this->NumberOfBuckets; ++i)
    {
    this->Buckets[i]->Delete();
    }
  delete [] this->Buckets;
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
void vtkHashMap<KeyType,DataType>::CheckLoadFactor()
{
  if(this->MaximumLoadFactor == 0) { return; }
  float loadFactor = float(this->NumberOfItems) / float(this->NumberOfBuckets);
  if(loadFactor > this->MaximumLoadFactor)
    {
    // Load factor is too big.  Increase the number of buckets.
    this->RehashItems((this->NumberOfBuckets*2)+7);
    }
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
void vtkHashMap<KeyType,DataType>::RehashItems(vtkIdType newNumberOfBuckets)
{
  if(newNumberOfBuckets < 1) { return; }
  if(this->NumberOfBuckets == newNumberOfBuckets) { return; }
  
  // Create new buckets.
  BucketType** newBuckets = new BucketType*[newNumberOfBuckets];
  vtkIdType i;
  for(i=0;i < newNumberOfBuckets;++i)
    {
    newBuckets[i] = BucketType::New();
    }
  
  // Re-hash the items.
  ItemType item;
  vtkIdType bucket;
  IteratorType* it = this->NewIterator();
  while(!it->IsDoneWithTraversal())
    {
    it->Iterator->GetData(item);
    bucket = vtkHashMapHashMethod(item.Key) % newNumberOfBuckets;
    newBuckets[bucket]->AppendItem(item);
    it->GoToNextItem();
    }
  it->Delete();
  
  // Delete the old buckets.
  for(i=0; i < this->NumberOfBuckets; ++i)
    {
    this->Buckets[i]->Delete();
    }
  delete [] this->Buckets;
  
  // Save the new buckets.
  this->Buckets = newBuckets;
  this->NumberOfBuckets = newNumberOfBuckets;
}

//----------------------------------------------------------------------------

#if defined ( _MSC_VER )
template <class KeyType,class DataType>
vtkHashMap<KeyType,DataType>::vtkHashMap(const vtkHashMap<KeyType,DataType>&){}
template <class KeyType,class DataType>
void vtkHashMap<KeyType,DataType>::operator=(const vtkHashMap<KeyType,DataType>&){}
#endif

#endif
