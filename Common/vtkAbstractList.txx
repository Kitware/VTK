/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractList.txx
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
#ifndef __vtkAbstractList_txx
#define __vtkAbstractList_txx

#include "vtkAbstractList.h"
#include "vtkString.h"

// Description:
// This is a default create function. All it does is to assign k2 to k1.
// The object has to be able to understand the assignment operator.
template<class DType>
static void vtkAbstractListDefaultCreateFunction(DType& k1, const DType& k2) 
{ k1 = k2; }

// Description:
// This is a default compare function. The object has to be able to 
// understand comparison operators <, == and >
template<class DType>
static int vtkAbstractListDefaultCompareFunction(const DType& k1, 
                                                const DType& k2)
{ 
  return ( k1 < k2 ) ? ( -1 ) : ( ( k1 == k2 ) ? ( 0 ) : ( 1 ) );
}

// Description:
// This is a default delete function. Since for most stack objects
// we do not have to do anyting on deletion, this function is empty.
template<class DType>
static void vtkAbstractListDefaultDeleteFunction(DType&) {};

// Description:
// This is a comparison function for C string keys or data.
template<class DType>
static int vtkAbstractListStringCompareFunction(const DType& k1, 
                                               const DType& k2)
{
  return strcmp(k1, k2);
}

// Description:
// This is a delete function for C string keys and data.
template<class DType>
static void vtkAbstractListStringDeleteFunction(DType& k1)
{
  char *key = const_cast<char*>( k1 );
  delete [] key;
}

// Description:
// This is a create function for C string keys and data.
template<class DType>
static void vtkAbstractListStringCreateFunction(DType& k1, 
                                               const DType& k2)
{
  char *tmp = vtkString::Duplicate(k2);
  k1 = tmp;
}
 
// Description:
// This is a delete function for reference counted objects.
template<class DType>
static void vtkAbstractListReferenceCountedDeleteFunction(DType& k1)
{
  k1->UnRegister(0);
}

// Description:
// This is a create function for reference counted objects.
template<class DType>
static void vtkAbstractListReferenceCountedCreateFunction(DType& k1, 
                                                         const DType& k2)
{
  k1 = k2;
  k1->Register(0);
}
 
// Description:
// If the key is C string, we assign the auxilary function pointers 
// to string functions.
template<class DataType>
void vtkAbstractListDataIsString(vtkAbstractList<DataType>* me)
{
  me->SetCompareFunction(vtkAbstractListStringCompareFunction);
  me->SetDeleteFunction(vtkAbstractListStringDeleteFunction);
  me->SetCreateFunction(vtkAbstractListStringCreateFunction); 
}
 
// Description:
// If the data is reference counted, we assign the auxilary function 
// pointers to reference counted functions.
// Note that we can mostly compare pointers as integers, so we do not
// have to set the compare function.
template<class DataType>
void vtkAbstractListDataIsReferenceCounted(vtkAbstractList<DataType>* me)
{
  me->SetDeleteFunction(vtkAbstractListReferenceCountedDeleteFunction);
  me->SetCreateFunction(vtkAbstractListReferenceCountedCreateFunction); 
}

template<class DataType>
vtkAbstractList<DataType>::vtkAbstractList()
{
  this->CompareFunction = vtkAbstractListDefaultCompareFunction;
  this->DeleteFunction  = vtkAbstractListDefaultDeleteFunction;
  this->CreateFunction  = vtkAbstractListDefaultCreateFunction;
}

#endif
