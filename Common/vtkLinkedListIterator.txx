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
  if ( !this->Container )
    {
    vtkGenericWarningMacro("No container set");
    return;
    }
  vtkLinkedList<DType> *llist 
    = static_cast<vtkLinkedList<DType>*>(this->Container);
  this->Pointer = llist->Head;
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

// Description:
// Check if the iterator is at the end of the container. Return 
// VTK_OK if it is.
template<class DType>
int vtkLinkedListIterator<DType>::IsDoneWithTraversal()
{
  if ( !this->Pointer )
    {
    return 1;
    }
  return 0;
}

// Description:
// Increment the iterator to the next location.
// Return VTK_OK if everything is ok.
template<class DType>
int vtkLinkedListIterator<DType>::GoToNextItem()
{
  if ( !this->Pointer )
    {
    return VTK_ERROR;
    }
  this->Pointer = this->Pointer->Next;
  return VTK_OK;
}

// Description:
// Decrement the iterator to the next location.
// Return VTK_OK if everything is ok.
template<class DType>
int vtkLinkedListIterator<DType>::GoToPreviousItem()
{
  if ( !this->Pointer )
    {
    return VTK_ERROR;
    }
  vtkLinkedList<DType> *llist 
    = static_cast<vtkLinkedList<DType>*>(this->Container);

  // Fast exit if at beginning of the list
  if ( this->Pointer == llist->Head )
    {
    this->Pointer = 0;
    return VTK_OK;
    }

  // Traverse the list to find the pervious node
  vtkLinkedListNode<DType> *curr = 0;
  for ( curr = llist->Head; curr && curr->Next; curr = curr->Next )
    {
    if ( curr->Next == this->Pointer )
      {
      this->Pointer = curr;
      return VTK_OK;
      }
    }
  return VTK_ERROR;
}

// Description:
// Increment the iterator to the next location.
// Return VTK_OK if everything is ok.
template<class DType>
int vtkLinkedListIterator<DType>::GoToLastItem()
{
  if ( !this->Pointer )
    {
    return VTK_ERROR;
    }
  vtkLinkedList<DType> *llist 
    = static_cast<vtkLinkedList<DType>*>(this->Container);
  this->Pointer = llist->Tail;
  if ( !this->Pointer )
    {
    return VTK_ERROR;
    }
  return VTK_OK;
}

#if defined ( _MSC_VER )
template <class DType>
vtkLinkedListIterator<DType>::vtkLinkedListIterator(const vtkLinkedListIterator<DType>&){}
template <class DType>
void vtkLinkedListIterator<DType>::operator=(const vtkLinkedListIterator<DType>&){}
#endif

#endif
