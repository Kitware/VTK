/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorIterator.txx
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

#ifndef __vtkVectorIterator_txx
#define __vtkVectorIterator_txx

#include "vtkVectorIterator.h"
#include "vtkAbstractIterator.txx"
#include "vtkVector.h"

template <class DType>
vtkVectorIterator<DType> *vtkVectorIterator<DType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkVectorIterator");
#endif
  return new vtkVectorIterator<DType>(); 
}

template<class DType>
void vtkVectorIterator<DType>::InitTraversal()
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
template<class DType>
int vtkVectorIterator<DType>::GetKey(vtkIdType& key)
{
  vtkVector<DType> *llist 
    = static_cast<vtkVector<DType>*>(this->Container);
  if ( this->Index >= llist->GetNumberOfItems() )
    {
    return VTK_ERROR;
    }
  key = this->Index;
  return VTK_OK;
}

// Description:
// Retrieve the data from the iterator. 
// This method returns VTK_OK if key was retrieved correctly.
template<class DType>
int vtkVectorIterator<DType>::GetData(DType& data)
{
  vtkVector<DType> *llist 
    = static_cast<vtkVector<DType>*>(this->Container);
  if ( this->Index >= llist->GetNumberOfItems() )
    {
    return VTK_ERROR;
    }
  data = llist->Array[this->Index];
  return VTK_OK;
}

// Description:
// Check if the iterator is at the end of the container. Return 
// VTK_OK if it is.
template<class DType>
int vtkVectorIterator<DType>::IsDoneWithTraversal()
{
  vtkVector<DType> *llist 
    = static_cast<vtkVector<DType>*>(this->Container);
  if ( this->Index >= llist->GetNumberOfItems() )
    {
    return VTK_OK;
    }
  return VTK_ERROR;
}

// Description:
// Increment the iterator to the next location.
// Return VTK_OK if everything is ok.
template<class DType>
int vtkVectorIterator<DType>::GoToNextItem()
{
  vtkVector<DType> *llist 
    = static_cast<vtkVector<DType>*>(this->Container);
  if ( this->Index >= llist->GetNumberOfItems() )
    {
    return VTK_ERROR;
    }
  this->Index++;
  return VTK_OK;
}

// Description:
// Decrement the iterator to the next location.
// Return VTK_OK if everything is ok.
template<class DType>
int vtkVectorIterator<DType>::GoToPreviousItem()
{
  vtkVector<DType> *llist 
    = static_cast<vtkVector<DType>*>(this->Container);
  if ( this->Index < 0 || this->Index >= llist->GetNumberOfItems() )
    {
    return VTK_ERROR;
    }
  if ( this->Index == 0 )
    {
    this->Index = llist->GetNumberOfItems();
    return VTK_OK;
    }
  this->Index--;
  return VTK_OK;
}

template<class DType>
int vtkVectorIterator<DType>::GoToLastItem()
{
  if ( !this->Container )
    {
    //cout << "No container" << endl;
    return VTK_ERROR;
    }
  vtkVector<DType> *llist 
    = static_cast<vtkVector<DType>*>(this->Container);
  if ( llist->GetNumberOfItems() <= 0 )
    {
    return VTK_ERROR;
    }
  
  this->Index = llist->GetNumberOfItems()-1;  
  return VTK_OK;
}

#if defined ( _MSC_VER )
template <class DType>
vtkVectorIterator<DType>::vtkVectorIterator(const vtkVectorIterator<DType>&){}
template <class DType>
void vtkVectorIterator<DType>::operator=(const vtkVectorIterator<DType>&){}
#endif

#endif
