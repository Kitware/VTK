/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayMap.txx
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
// Include blockers needed since vtkVector.h includes this file
// when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.
#ifndef __vtkArrayMap_txx
#define __vtkArrayMap_txx

#include "vtkArrayMap.h"
#include "vtkVector.txx"
#include "vtkAbstractMap.txx"
#include "vtkArrayMapIterator.txx"

template <class KeyType,class DataType>
vtkArrayMap<KeyType,DataType> *vtkArrayMap<KeyType,DataType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkArrayMap");
#endif
  return new vtkArrayMap<KeyType,DataType>(); 
}

// Description:
// Sets the item at with specific key to data.
// It overwrites the old item.
// It returns VTK_OK if successfull.
template<class KeyType, class DataType>
int vtkArrayMap<KeyType,DataType>::SetItem(const KeyType& key, 
                                           const DataType& data)
{
  //vtkAbstractMapItem<KeyType,DataType>
  vtkAbstractMapItem<KeyType,DataType> *item = this->FindDataItem(key);
  if ( item )
    {
    vtkContainerDeleteMethod(item->Data);
    item->Data = static_cast<DataType>(vtkContainerCreateMethod(data));
    return VTK_OK;
    }
  if ( !this->Array )
    {
    this->Array = vtkVector< vtkAbstractMapItem<KeyType,DataType>* >::New();
    }
  if ( !this->Array )
    {
    return VTK_ERROR;
    }
  item = new vtkAbstractMapItem<KeyType,DataType>;
  item->Key  = static_cast<KeyType>(vtkContainerCreateMethod(key));
  item->Data = static_cast<DataType>(vtkContainerCreateMethod(data));
  this->Array->AppendItem(item);
  return VTK_OK;
  
}

// Description:
// Remove an Item with the key from the map.
// It returns VTK_OK if successfull.
template<class KeyType, class DataType>
int vtkArrayMap<KeyType,DataType>::RemoveItem(const KeyType& key)
{
  if ( !this->Array )
    {
    return 0;
    }
  vtkAbstractMapItem<KeyType,DataType> *item;
  vtkIdType cc;
  for ( cc = 0; cc <= this->Array->GetNumberOfItems(); cc ++ )
    {
    this->Array->GetItemNoCheck(cc, item);
    if ( vtkContainerCompareMethod(key, item->Key) == 0 )
      {
      this->Array->RemoveItem(cc);
      vtkContainerDeleteMethod(item->Key);
      vtkContainerDeleteMethod(item->Data);
      delete item;
      return VTK_OK;
      }
    }
  return VTK_ERROR;
}

// Description:
// Remove all items from the map.
template<class KeyType, class DataType>
void vtkArrayMap<KeyType,DataType>::RemoveAllItems()
{
  if ( this->Array )
    {
    vtkIdType cc;
    for ( cc=0; cc < this->Array->GetNumberOfItems(); cc ++ )
      {
      vtkAbstractMapItem<KeyType,DataType> *item;
      this->Array->GetItemNoCheck(cc, item);
      vtkContainerDeleteMethod(item->Key);
      vtkContainerDeleteMethod(item->Data);
      delete item;
      }
    this->Array->Delete(); 
    }
}

// Description:
// Return the data asociated with the key.
// It returns VTK_OK if successfull.
template<class KeyType, class DataType>
int vtkArrayMap<KeyType,DataType>::GetItem(const KeyType& key, DataType& data)
{
  vtkAbstractMapItem<KeyType,DataType> *item = this->FindDataItem(key);
  if ( item )
    {
    data = item->Data;
    return VTK_OK;
    }
  return VTK_ERROR;
}

template<class KeyType, class DataType>
vtkAbstractMapItem<KeyType,DataType> *
vtkArrayMap<KeyType,DataType>::FindDataItem(KeyType key)
{
  if ( !this->Array )
    {
    return 0;
    }
  vtkAbstractMapItem<KeyType,DataType> *item;
  vtkIdType cc;
  for ( cc = 0; cc < this->Array->GetNumberOfItems(); cc ++ )
    {
    this->Array->GetItemNoCheck(cc, item);
    if ( vtkContainerCompareMethod(key, item->Key) == 0 )
      {
      return item;
      }
    }
  return 0;
}
  // Description:
  // Return the number of items currently held in this container. This
  // different from GetSize which is provided for some containers. GetSize
  // will return how many items the container can currently hold.
template<class KeyType, class DataType>
vtkIdType vtkArrayMap<KeyType,DataType>::GetNumberOfItems() const
{
  if ( !this->Array )
    {
    return 0;
    }
  return this->Array->GetNumberOfItems();
}


template<class KeyType, class DataType>
vtkArrayMap<KeyType,DataType>::~vtkArrayMap() 
{ 
  if ( this->Array )
    {
    vtkIdType cc;
    for ( cc=0; cc < this->Array->GetNumberOfItems(); cc ++ )
      {
      vtkAbstractMapItem<KeyType,DataType> *item;
      this->Array->GetItemNoCheck(cc, item);
      vtkContainerDeleteMethod(item->Key);
      vtkContainerDeleteMethod(item->Data);
      delete item;
      }
    this->Array->Delete(); 
    }
}

template<class KeyType, class DataType>
void vtkArrayMap<KeyType,DataType>::DebugList()
{
  if ( this->Array )
    {
    this->Array->DebugList();
    }
}

template <class KeyType, class DataType>
vtkArrayMapIterator<KeyType,DataType> *
vtkArrayMap<KeyType,DataType>::NewIterator()
{
  vtkArrayMapIterator<KeyType,DataType> *it 
    = vtkArrayMapIterator<KeyType,DataType>::New();
  it->SetContainer(this);
  it->InitTraversal();
  return it;
}

#if defined ( _MSC_VER )
template <class KeyType,class DataType>
vtkArrayMap<KeyType,DataType>::vtkArrayMap(const vtkArrayMap<KeyType,DataType>&){}
template <class KeyType,class DataType>
void vtkArrayMap<KeyType,DataType>::operator=(const vtkArrayMap<KeyType,DataType>&){}
#endif

#endif


