/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHashMapIterator.txx
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

#ifndef __vtkHashMapIterator_txx
#define __vtkHashMapIterator_txx

#include "vtkHashMapIterator.h"
#include "vtkAbstractIterator.txx"
#include "vtkHashMap.h"

//----------------------------------------------------------------------------
template <class KeyType,class DataType>
vtkHashMapIterator<KeyType,DataType> *vtkHashMapIterator<KeyType,DataType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkHashMapIterator");
#endif
  return new vtkHashMapIterator<KeyType,DataType>(); 
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::InitTraversal()
{
  vtkHashMap<KeyType,DataType>* hmap 
    = static_cast<vtkHashMap<KeyType,DataType>*>(this->Container);
  this->Iterator = hmap->Buckets[0]->NewIterator();
  this->GoToFirstItem();
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
int vtkHashMapIterator<KeyType,DataType>::GetKey(KeyType& key)
{
  if(this->IsDoneWithTraversal()) { return VTK_ERROR; }
  ItemType item;
  if(this->Iterator->GetData(item) == VTK_OK)
    {
    key = item.Key;
    return VTK_OK;
    }
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
int vtkHashMapIterator<KeyType,DataType>::GetData(DataType& data)
{
  if(this->IsDoneWithTraversal()) { return VTK_ERROR; }
  ItemType item;
  if(this->Iterator->GetData(item) == VTK_OK)
    {
    data = item.Data;
    return VTK_OK;
    }
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
int vtkHashMapIterator<KeyType,DataType>::GetKeyAndData(KeyType& key,
                                                        DataType& data)
{
  if(this->IsDoneWithTraversal()) { return VTK_ERROR; }
  ItemType item;
  if(this->Iterator->GetData(item) == VTK_OK)
    {
    key = item.Key;
    data = item.Data;
    return VTK_OK;
    }
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
int vtkHashMapIterator<KeyType,DataType>::IsDoneWithTraversal()
{
  vtkHashMap<KeyType,DataType>* hmap 
    = static_cast<vtkHashMap<KeyType,DataType>*>(this->Container);
  return (this->Bucket == hmap->NumberOfBuckets) ? 1:0;
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::GoToNextItem()
{
  if(this->IsDoneWithTraversal()) { this->InitTraversal(); }
  this->Iterator->GoToNextItem();
  this->ScanForward();
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::GoToPreviousItem()
{
  if(this->IsDoneWithTraversal())
    {
    this->GoToLastItem();
    }
  else
    {
    this->Iterator->GoToPreviousItem();
    this->ScanBackward();
    }
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::GoToFirstItem()
{
  vtkHashMap<KeyType,DataType>* hmap 
    = static_cast<vtkHashMap<KeyType,DataType>*>(this->Container);
  this->Bucket = 0;
  this->Iterator->SetContainer(hmap->Buckets[this->Bucket]);
  this->Iterator->InitTraversal();
  this->ScanForward();
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::GoToLastItem()
{
  vtkHashMap<KeyType,DataType>* hmap 
  = static_cast<vtkHashMap<KeyType,DataType>*>(this->Container);
  this->Bucket = hmap->NumberOfBuckets-1;
  this->Iterator->SetContainer(hmap->Buckets[this->Bucket]);
  this->Iterator->GoToLastItem();
  this->ScanBackward();
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
vtkHashMapIterator<KeyType,DataType>::vtkHashMapIterator()
{
  this->Iterator = 0;
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
vtkHashMapIterator<KeyType,DataType>::~vtkHashMapIterator()
{
  if(this->Iterator) { this->Iterator->Delete(); }
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::ScanForward()
{
  // Move the iterator forward until a valid item is reached.
  vtkHashMap<KeyType,DataType>* hmap 
    = static_cast<vtkHashMap<KeyType,DataType>*>(this->Container);
  
  while(this->Iterator->IsDoneWithTraversal() &&
        (++this->Bucket < hmap->NumberOfBuckets))
    {
    this->Iterator->SetContainer(hmap->Buckets[this->Bucket]);
    this->Iterator->InitTraversal();
    }
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::ScanBackward()
{
  // Move the iterator backward until a valid item is reached.
  vtkHashMap<KeyType,DataType>* hmap 
    = static_cast<vtkHashMap<KeyType,DataType>*>(this->Container);
  
  while(this->Iterator->IsDoneWithTraversal() &&
        (this->Bucket > 0))
    {
    this->Iterator->SetContainer(hmap->Buckets[--this->Bucket]);
    this->Iterator->GoToLastItem();
    }  
}

//----------------------------------------------------------------------------

#if defined ( _MSC_VER )
template <class KeyType,class DataType>
vtkHashMapIterator<KeyType,DataType>::vtkHashMapIterator(const vtkHashMapIterator<KeyType,DataType>&){}
template <class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::operator=(const vtkHashMapIterator<KeyType,DataType>&){}
#endif

#endif
