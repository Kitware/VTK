/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractMap.txx
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
#ifndef __vtkAbstractMap_txx
#define __vtkAbstractMap_txx

#include "vtkAbstractMap.h"
#include "vtkString.h"

// Description:
// This is a default create function. All it does is to assign k2 to k1.
// The object has to be able to understand the assignment operator.
template<class DType>
static void vtkAbstractMapDefaultCreateFunction(DType& k1, const DType& k2) 
{ k1 = k2; }

// Description:
// This is a default compare function. The object has to be able to 
// understand comparison operators <, == and >
template<class DType>
static int vtkAbstractMapDefaultCompareFunction(const DType& k1, 
                                                const DType& k2)
{ 
  return ( k1 < k2 ) ? ( -1 ) : ( ( k1 == k2 ) ? ( 0 ) : ( 1 ) );
}

// Description:
// This is a default delete function. Since for most stack objects
// we do not have to do anyting on deletion, this function is empty.
template<class DType>
static void vtkAbstractMapDefaultDeleteFunction(DType&) {};


// Since auxilary functions are static, we have to assign them
// default values.
template<class KeyType, class DataType>
vtkAbstractMap<KeyType,DataType>::KeyCreateFunctionType
vtkAbstractMap<KeyType,DataType>::KeyCreateFunction
= &vtkAbstractMapDefaultCreateFunction;

template<class KeyType, class DataType>
vtkAbstractMap<KeyType,DataType>::KeyCompareFunctionType
vtkAbstractMap<KeyType,DataType>::KeyCompareFunction
= &vtkAbstractMapDefaultCompareFunction;

template<class KeyType, class DataType>
vtkAbstractMap<KeyType,DataType>::KeyDeleteFunctionType
vtkAbstractMap<KeyType,DataType>::KeyDeleteFunction
= &vtkAbstractMapDefaultDeleteFunction;

template<class KeyType, class DataType>
vtkAbstractMap<KeyType,DataType>::DataCreateFunctionType
vtkAbstractMap<KeyType,DataType>::DataCreateFunction
= &vtkAbstractMapDefaultCreateFunction;

template<class KeyType, class DataType>
vtkAbstractMap<KeyType,DataType>::DataCompareFunctionType
vtkAbstractMap<KeyType,DataType>::DataCompareFunction
= &vtkAbstractMapDefaultCompareFunction;

template<class KeyType, class DataType>
vtkAbstractMap<KeyType,DataType>::DataDeleteFunctionType
vtkAbstractMap<KeyType,DataType>::DataDeleteFunction
= &vtkAbstractMapDefaultDeleteFunction;


// Description:
// This is a comparison function for C string keys or data.
template<class DType>
static int vtkAbstractMapStringCompareFunction(const DType& k1, 
                                               const DType& k2)
{
  return strcmp(k1, k2);
}

// Description:
// This is a delete function for C string keys and data.
template<class DType>
static void vtkAbstractMapStringDeleteFunction(DType& k1)
{
  char *key = const_cast<char*>( k1 );
  delete [] key;
}

// Description:
// This is a create function for C string keys and data.
template<class DType>
static void vtkAbstractMapStringCreateFunction(DType& k1, 
                                               const DType& k2)
{
  char *tmp = vtkString::Duplicate(k2);
  k1 = tmp;
}
 
// Description:
// This is a delete function for reference counted objects.
template<class DType>
static void vtkAbstractMapReferenceCountedDeleteFunction(DType& k1)
{
  k1->UnRegister(0);
}

// Description:
// This is a create function for reference counted objects.
template<class DType>
static void vtkAbstractMapReferenceCountedCreateFunction(DType& k1, 
                                                         const DType& k2)
{
  k1 = k2;
  k1->Register(0);
}
 
// Description:
// If the key is C string, we assign the auxilary function pointers 
// to string functions.
template<class KeyType, class DataType>
void vtkAbstractMapKeyIsString(const vtkAbstractMap<KeyType,DataType>*)
{
  typedef vtkAbstractMap<KeyType,DataType> Superclass;
  Superclass::SetKeyCompareFunction(vtkAbstractMapStringCompareFunction);
  Superclass::SetKeyDeleteFunction(vtkAbstractMapStringDeleteFunction);
  Superclass::SetKeyCreateFunction(vtkAbstractMapStringCreateFunction); 
}
 
// Description:
// If the data is reference counted, we assign the auxilary function 
// pointers to reference counted functions.
// Note that we can mostly compare pointers as integers, so we do not
// have to set the compare function.
template<class KeyType, class DataType>
void vtkAbstractMapDataIsReferenceCounted(
  const vtkAbstractMap<KeyType,DataType>*)
{
  typedef vtkAbstractMap<KeyType,DataType> Superclass;
  Superclass::SetDataDeleteFunction(
    vtkAbstractMapReferenceCountedDeleteFunction);
  Superclass::SetDataCreateFunction(
    vtkAbstractMapReferenceCountedCreateFunction); 
}
#endif
