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
  if ( !this->Container )
    {
    //cout << "No container" << endl;
    return;
    }
  this->Index = 0;
}

// Description:
// Retrieve the index of the element.
// This method returns VTK_OK if key was retrieved correctly.
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

// Description:
// Retrieve the data from the iterator. 
// This method returns VTK_OK if key was retrieved correctly.
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

// Description:
// Check if the iterator is at the end of the container. Return 
// VTK_OK if it is.
template<class KeyType,class DataType>
int vtkArrayMapIterator<KeyType,DataType>::IsDoneWithTraversal()
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  if ( !lmap || this->Index < 0 || this->Index >= lmap->GetNumberOfItems() )
    {
    return VTK_OK;
    }
  return VTK_ERROR;
}

// Description:
// Increment the iterator to the next location.
// Return VTK_OK if everything is ok.
template<class KeyType,class DataType>
int vtkArrayMapIterator<KeyType,DataType>::GoToNextItem()
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  if ( !lmap || this->Index >= lmap->GetNumberOfItems() )
    {
    return VTK_ERROR;
    }
  this->Index++;
  return VTK_OK;
}

// Description:
// Decrement the iterator to the next location.
// Return VTK_OK if everything is ok.
template<class KeyType,class DataType>
int vtkArrayMapIterator<KeyType,DataType>::GoToPreviousItem()
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  if ( !lmap || this->Index < 0 || this->Index >= lmap->GetNumberOfItems() )
    {
    return VTK_ERROR;
    }
  if ( this->Index == 0 )
    {
    this->Index = lmap->GetNumberOfItems();
    return VTK_OK;
    }
  this->Index--;
  return VTK_OK;
}

// Description:
// Go to the "first" item of the map.
// Return VTK_OK if everything is ok.
template<class KeyType,class DataType>
int vtkArrayMapIterator<KeyType,DataType>::GoToFirstItem()
{
  this->InitTraversal();
  return VTK_OK;
}

// Description:
// Go to the "last" item of the map.
// Return VTK_OK if everything is ok.
template<class KeyType,class DataType>
int vtkArrayMapIterator<KeyType,DataType>::GoToLastItem()
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  if ( lmap->GetNumberOfItems() <= 0 )
    {
    return VTK_ERROR;
    }
  this->Index = lmap->GetNumberOfItems()-1;
  return VTK_OK;
}

#if defined ( _MSC_VER )
template <class KeyType,class DataType>
vtkArrayMapIterator<KeyType,DataType>::vtkArrayMapIterator(const vtkArrayMapIterator<KeyType,DataType>&){}
template <class KeyType,class DataType>
void vtkArrayMapIterator<KeyType,DataType>::operator=(const vtkArrayMapIterator<KeyType,DataType>&){}
#endif

#endif
