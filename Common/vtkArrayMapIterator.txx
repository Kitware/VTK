/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayMapIterator.txx
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
// Include blockers needed since vtkArrayMap.h includes this file
// when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.

#ifndef __vtkArrayMapIterator_txx
#define __vtkArrayMapIterator_txx

#include "vtkArrayMapIterator.h"
#include "vtkAbstractIterator.txx"
#include "vtkArrayMap.h"

template <class KeyType,class DataType>
vtkArrayMapIterator<KeyType,DataType> *vtkArrayMapIterator<KeyType,DataType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkArrayMapIterator");
#endif
  return new vtkArrayMapIterator<KeyType,DataType>(); 
}

template<class KeyType,class DataType>
void vtkArrayMapIterator<KeyType,DataType>::InitTraversal()
{
  this->GoToFirstItem();
}

template<class KeyType,class DataType>
int vtkArrayMapIterator<KeyType,DataType>::GetKey(KeyType& key)
{  
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  vtkAbstractMapItem<KeyType,DataType> *item = 0;
  if ( !lmap || lmap->Array->GetItem(this->Index, item) != VTK_OK )
    {
    return VTK_ERROR;
    }
  key = item->Key;
  return VTK_OK;
}

template<class KeyType,class DataType>
int vtkArrayMapIterator<KeyType,DataType>::GetData(DataType& data)
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  vtkAbstractMapItem<KeyType,DataType> *item = 0;
  if ( !lmap || lmap->Array->GetItem(this->Index, item) != VTK_OK )
    {
    return VTK_ERROR;
    }
  data = item->Data;
  return VTK_OK;
}

template<class KeyType,class DataType>
int vtkArrayMapIterator<KeyType,DataType>::IsDoneWithTraversal()
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  if ( !lmap || this->Index < 0 || this->Index >= lmap->GetNumberOfItems() )
    {
    return 1;
    }
  return 0;
}

template<class KeyType,class DataType>
void vtkArrayMapIterator<KeyType,DataType>::GoToNextItem()
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  if(this->Index < lmap->GetNumberOfItems())
    {
    ++this->Index;
    }
  else
    {
    this->Index = 0;
    }
}

template<class KeyType,class DataType>
void vtkArrayMapIterator<KeyType,DataType>::GoToPreviousItem()
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  if(this->Index > 0)
    {
    --this->Index;
    }
  else
    {
    this->Index = lmap->GetNumberOfItems();
    }
}

template<class KeyType,class DataType>
void vtkArrayMapIterator<KeyType,DataType>::GoToFirstItem()
{
  this->Index = 0;
}

template<class KeyType,class DataType>
void vtkArrayMapIterator<KeyType,DataType>::GoToLastItem()
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  if(lmap->GetNumberOfItems() > 0)
    {
    this->Index = lmap->GetNumberOfItems()-1;  
    }
  else
    {
    this->Index = 0;
    }
}

#if defined ( _MSC_VER )
template <class KeyType,class DataType>
vtkArrayMapIterator<KeyType,DataType>::vtkArrayMapIterator(const vtkArrayMapIterator<KeyType,DataType>&){}
template <class KeyType,class DataType>
void vtkArrayMapIterator<KeyType,DataType>::operator=(const vtkArrayMapIterator<KeyType,DataType>&){}
#endif

#endif
