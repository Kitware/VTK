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

//----------------------------------------------------------------------------
template <class DType>
vtkVectorIterator<DType> *vtkVectorIterator<DType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkVectorIterator");
#endif
  return new vtkVectorIterator<DType>(); 
}

//----------------------------------------------------------------------------
template<class DType>
void vtkVectorIterator<DType>::InitTraversal()
{
  this->GoToFirstItem();
}

//----------------------------------------------------------------------------
template<class DType>
int vtkVectorIterator<DType>::GetKey(vtkIdType& key)
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  if ( this->Index == llist->NumberOfItems ) { return VTK_ERROR; }
  key = this->Index;
  return VTK_OK;
}

//----------------------------------------------------------------------------
template<class DType>
int vtkVectorIterator<DType>::GetData(DType& data)
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  if ( this->Index == llist->NumberOfItems ) { return VTK_ERROR; }
  data = llist->Array[this->Index];
  return VTK_OK;
}

//----------------------------------------------------------------------------
template<class DType>
int vtkVectorIterator<DType>::SetData(const DType& data)
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  if ( this->Index == llist->NumberOfItems ) { return VTK_ERROR; }
  llist->Array[this->Index] = data;
  return VTK_OK;
}

//----------------------------------------------------------------------------
template<class DType>
int vtkVectorIterator<DType>::IsDoneWithTraversal()
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  return (this->Index == llist->NumberOfItems)? 1:0;
}

//----------------------------------------------------------------------------
template<class DType>
void vtkVectorIterator<DType>::GoToNextItem()
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  if(this->Index < llist->NumberOfItems)
    {
    ++this->Index;
    }
  else
    {
    this->Index = 0;
    }
}

//----------------------------------------------------------------------------
template<class DType>
void vtkVectorIterator<DType>::GoToPreviousItem()
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  if(this->Index > 0)
    {
    --this->Index;
    }
  else
    {
    this->Index = llist->NumberOfItems;
    }
}

//----------------------------------------------------------------------------
template<class DType>
void vtkVectorIterator<DType>::GoToFirstItem()
{
  this->Index = 0;
}

//----------------------------------------------------------------------------
template<class DType>
void vtkVectorIterator<DType>::GoToLastItem()
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  if(llist->NumberOfItems > 0)
    {
    this->Index = llist->NumberOfItems-1;  
    }
  else
    {
    this->Index = 0;
    }
}

//----------------------------------------------------------------------------

#if defined ( _MSC_VER )
template <class DType>
vtkVectorIterator<DType>::vtkVectorIterator(const vtkVectorIterator<DType>&){}
template <class DType>
void vtkVectorIterator<DType>::operator=(const vtkVectorIterator<DType>&){}
#endif

#endif
