/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinkedListIterator.txx
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

#ifndef __vtkLinkedListIterator_txx
#define __vtkLinkedListIterator_txx

#include "vtkLinkedListIterator.h"
#include "vtkAbstractIterator.txx"
#include "vtkLinkedList.h"

template <class DType>
vtkLinkedListIterator<DType> *vtkLinkedListIterator<DType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkLinkedListIterator");
#endif
  return new vtkLinkedListIterator<DType>(); 
}

template<class DType>
void vtkLinkedListIterator<DType>::InitTraversal()
{
  this->GoToFirstItem();
}

// Description:
// Retrieve the index of the element.
// This method returns VTK_OK if key was retrieved correctly.
template<class DType>
int vtkLinkedListIterator<DType>::GetKey(vtkIdType& key)
{
  if ( !this->Pointer )
    {
    return VTK_ERROR;
    }

  vtkLinkedListNode<DType> *curr;
  int cc = 0;
  vtkLinkedList<DType> *llist 
    = static_cast<vtkLinkedList<DType>*>(this->Container);
  for ( curr = llist->Head; curr; curr = curr->Next )
    {
    if ( curr == this->Pointer )
      {
      key = cc;
      return VTK_OK;
      }
    cc ++;
    }
  return VTK_ERROR;
}

// Description:
// Retrieve the data from the iterator. 
// This method returns VTK_OK if key was retrieved correctly.
template<class DType>
int vtkLinkedListIterator<DType>::GetData(DType& data)
{
  if ( !this->Pointer )
    {
    return VTK_ERROR;
    }
  data = this->Pointer->Data;
  return VTK_OK;
}

template<class DType>
int vtkLinkedListIterator<DType>::IsDoneWithTraversal()
{
  if ( !this->Pointer )
    {
    return 1;
    }
  return 0;
}

template<class DType>
void vtkLinkedListIterator<DType>::GoToNextItem()
{
  if(this->IsDoneWithTraversal())
    {
    this->GoToFirstItem();
    }
  else
    {
    this->Pointer = this->Pointer->Next;
    }
}

template<class DType>
void vtkLinkedListIterator<DType>::GoToPreviousItem()
{
  if(this->IsDoneWithTraversal())
    {
    this->GoToLastItem();
    return;
    }
  
  vtkLinkedList<DType> *llist 
    = static_cast<vtkLinkedList<DType>*>(this->Container);
  
  // Fast exit if at beginning of the list
  if ( this->Pointer == llist->Head )
    {
    this->Pointer = 0;
    return;
    }

  // Traverse the list to find the previous node
  vtkLinkedListNode<DType> *curr = 0;
  for ( curr = llist->Head; curr ; curr = curr->Next )
    {
    if ( curr->Next == this->Pointer )
      {
      break;
      }
    }
  
  this->Pointer = curr;
}

template<class DType>
void vtkLinkedListIterator<DType>::GoToFirstItem()
{
  vtkLinkedList<DType> *llist 
    = static_cast<vtkLinkedList<DType>*>(this->Container);
  this->Pointer = llist->Head;
}

template<class DType>
void vtkLinkedListIterator<DType>::GoToLastItem()
{
  vtkLinkedList<DType> *llist 
    = static_cast<vtkLinkedList<DType>*>(this->Container);
  this->Pointer = llist->Tail;
}

#if defined ( _MSC_VER )
template <class DType>
vtkLinkedListIterator<DType>::vtkLinkedListIterator(const vtkLinkedListIterator<DType>&){}
template <class DType>
void vtkLinkedListIterator<DType>::operator=(const vtkLinkedListIterator<DType>&){}
#endif

#endif
