/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSortDataArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkSortDataArray.h"

#include "vtkAbstractArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <algorithm>

// -------------------------------------------------------------------------

vtkStandardNewMacro(vtkSortDataArray);

vtkSortDataArray::vtkSortDataArray()
{
}

vtkSortDataArray::~vtkSortDataArray()
{
}

void vtkSortDataArray::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ---------------------------------------------------------------------------
// Sorting templates using operator <

template<class TKey, class TValue>
inline void vtkSortDataArraySwap(TKey *keys, TValue *values, int tupleSize,
                                 vtkIdType index1, vtkIdType index2)
{
  TKey tmpkey;
  TValue tmpvalue;
  TKey *k1 = keys + index1;
  TValue *v1 = values + index1*tupleSize;
  TKey *k2 = keys + index2;
  TValue *v2 = values + index2*tupleSize;

  tmpkey = *k1;
  *k1 = *k2;
  *k2 = tmpkey;

  for (int i = 0; i < tupleSize; i++)
    {
    tmpvalue = v1[i];
    v1[i] = v2[i];
    v2[i] = tmpvalue;
    }
}

template<class TKey, class TValue>
void vtkSortDataArrayBubbleSort(TKey *keys, TValue *values,
                                vtkIdType size, int tupleSize)
{
  for (int i = 1; i < size; i++)
    {
    for (int j = i; (j > 0) && (keys[j] < keys[j-1]); j--)
      {
      vtkSortDataArraySwap(keys, values, tupleSize, j, j-1);
      }
    }
}

template<class TKey, class TValue>
void vtkSortDataArrayQuickSort(TKey *keys, TValue *values,
                               vtkIdType size, int tupleSize)
{
  while (1)
    {
    if (size < 8)
      {
      vtkSortDataArrayBubbleSort(keys, values, size, tupleSize);
      return;
      }

    vtkIdType pivot = static_cast<vtkIdType>(vtkMath::Random(0,
                                                             static_cast<double>(size)));
    //vtkIdType pivot = size/2;
    vtkSortDataArraySwap(keys, values, tupleSize, 0, pivot);
    // Pivot now stored at index 0.

    vtkIdType left = 1;
    vtkIdType right = size - 1;
    while (1)
      {
      while ((left <= right) && (keys[left] <= keys[0])) left++;
      while ((left <= right) && (keys[right] >= keys[0])) right--;
      if (left > right) break;
      vtkSortDataArraySwap(keys, values, tupleSize, left, right);
      }

    // Place the pivot back in the middle
    vtkSortDataArraySwap(keys, values, tupleSize, 0, left-1);

    // Recurse
    vtkSortDataArrayQuickSort(keys + left, values + left*tupleSize,
                              size-left, tupleSize);
    size = left-1;
    }
}

// ---------------------------------------------------------------------------
// Sorting templates using user-defined comparison operation

template<class TKey, class TValue, class TComp>
void vtkSortDataArrayBubbleSort(TKey *keys, TValue *values,
                                vtkIdType size, int tupleSize, TComp comp)
{
  for (int i = 1; i < size; i++)
    {
    for (int j = i; (j > 0) && comp(keys[j],keys[j-1]); j--)
      {
      vtkSortDataArraySwap(keys, values, tupleSize, j, j-1);
      }
    }
}

template<class TKey, class TValue, class TComp>
void vtkSortDataArrayQuickSort(TKey *keys, TValue *values,
                               vtkIdType size, int tupleSize, TComp comp)
{
  while (1)
    {
    if (size < 8)
      {
      vtkSortDataArrayBubbleSort(keys, values, size, tupleSize, comp);
      return;
      }

    vtkIdType pivot = static_cast<vtkIdType>(vtkMath::Random(0,
                                                             static_cast<double>(size)));
    //vtkIdType pivot = size/2;
    vtkSortDataArraySwap(keys, values, tupleSize, 0, pivot);
    // Pivot now stored at index 0.

    vtkIdType left = 1;
    vtkIdType right = size - 1;
    while (1)
      {
      while ((left <= right) && !comp(keys[0],keys[left])) left++;
      while ((left <= right) && !comp(keys[right],keys[0])) right--;
      if (left > right) break;
      vtkSortDataArraySwap(keys, values, tupleSize, left, right);
      }

    // Place the pivot back in the middle
    vtkSortDataArraySwap(keys, values, tupleSize, 0, left-1);

    // Recurse
    vtkSortDataArrayQuickSort(keys + left, values + left*tupleSize,
                              size-left, tupleSize, comp);
    size = left-1;
    }
}

// ---------------------------------------------------------------------------
// Data array to raw array template helper functions

template<class TKey, class TValue, class TComp>
inline void vtkSortDataArraySort00(TKey *keys, TValue *values,
                                   vtkIdType array_size, int tuple_size, TComp comp)
{
  vtkSortDataArrayQuickSort(keys, values, array_size, tuple_size, comp);
}

template<class TKey, class TValue>
inline void vtkSortDataArraySort00(TKey *keys, TValue *values,
                                   vtkIdType array_size, int tuple_size)
{
  vtkSortDataArrayQuickSort(keys, values, array_size, tuple_size);
}

template<class TKey, class TComp>
void vtkSortDataArraySort01(TKey *keys, vtkAbstractArray *values, vtkIdType array_size,
                            TComp comp)
{
  if (array_size != values->GetNumberOfTuples())
    {
    vtkGenericWarningMacro("Could not sort arrays.  Key and value arrays have different sizes.");
    return;
    }

  switch (values->GetDataType())
    {
    vtkExtraExtendedTemplateMacro(
      vtkSortDataArraySort00(keys, static_cast<VTK_TT *>(values->GetVoidPointer(0)),
                             array_size, values->GetNumberOfComponents(), comp));
    }
}

template<class TKey>
void vtkSortDataArraySort01(TKey *keys, vtkAbstractArray *values, vtkIdType array_size)
{
  if (array_size != values->GetNumberOfTuples())
    {
    vtkGenericWarningMacro("Could not sort arrays.  Key and value arrays have different sizes.");
    return;
    }

  switch (values->GetDataType())
    {
    vtkExtraExtendedTemplateMacro(
      vtkSortDataArraySort00(keys, static_cast<VTK_TT *>(values->GetVoidPointer(0)),
                             array_size, values->GetNumberOfComponents()));
    }
}

template<class TValue>
void vtkSortDataArraySort10(vtkAbstractArray *keys, TValue *values,
                            vtkIdType array_size, int tuple_size)
{
  if (array_size != keys->GetNumberOfTuples())
    {
    vtkGenericWarningMacro("Could not sort arrays.  Key and value arrays have different sizes.");
    return;
    }

  if (keys->GetNumberOfComponents() != 1)
    {
    vtkGenericWarningMacro("Could not sort arrays.  Keys must be 1-tuples.");
    return;
    }

  switch (keys->GetDataType())
    {
    vtkExtendedTemplateMacro(
      vtkSortDataArraySort00(static_cast<VTK_TT*>(keys->GetVoidPointer(0)),
                             values, array_size, tuple_size));
    }
}

void vtkSortDataArraySort11(vtkAbstractArray *keys, vtkAbstractArray *values)
{
  switch (values->GetDataType())
    {
    vtkExtraExtendedTemplateMacro(
      vtkSortDataArraySort10(keys, static_cast<VTK_TT *>(values->GetVoidPointer(0)),
                             values->GetNumberOfTuples(),
                             values->GetNumberOfComponents()));
    }
}

// The component that qsort should use to compare tuples.
// This is ugly and not thread-safe but it works.
static int vtkSortDataArrayComp = 0;

// Key comparison functions for qsort must be in an extern "C"
// or some compilers will be unable to use their function pointers
// as arguments to qsort.
extern "C" {
int vtkSortDataArrayComponentCompare_VTK_DOUBLE( const void* a, const void* b )
{
  return ((double*)a)[vtkSortDataArrayComp] < ((double*)b)[vtkSortDataArrayComp] ? -1 :
    (((double*)a)[vtkSortDataArrayComp] == ((double*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_FLOAT( const void* a, const void* b )
{
  return ((float*)a)[vtkSortDataArrayComp] < ((float*)b)[vtkSortDataArrayComp] ? -1 :
    (((float*)a)[vtkSortDataArrayComp] == ((float*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

#ifdef VTK_TYPE_USE_LONG_LONG
int vtkSortDataArrayComponentCompare_VTK_LONG_LONG( const void* a, const void* b )
{
  return ((long long*)a)[vtkSortDataArrayComp] < ((long long*)b)[vtkSortDataArrayComp] ? -1 :
    (((long long*)a)[vtkSortDataArrayComp] == ((long long*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_UNSIGNED_LONG_LONG( const void* a, const void* b )
{
  return ((unsigned long long*)a)[vtkSortDataArrayComp] < ((unsigned long long*)b)[vtkSortDataArrayComp] ? -1 :
    (((unsigned long long*)a)[vtkSortDataArrayComp] == ((unsigned long long*)b)[vtkSortDataArrayComp] ? 0 : 1);
}
#endif // VTK_TYPE_USE_LONG_LONG

#ifdef VTK_TYPE_USE___INT64
int vtkSortDataArrayComponentCompare_VTK___INT64( const void* a, const void* b )
{
  return ((vtkTypeInt64*)a)[vtkSortDataArrayComp] < ((vtkTypeInt64*)b)[vtkSortDataArrayComp] ? -1 :
    (((vtkTypeInt64*)a)[vtkSortDataArrayComp] == ((vtkTypeInt64*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_UNSIGNED___INT64( const void* a, const void* b )
{
  return ((vtkTypeUInt64*)a)[vtkSortDataArrayComp] < ((vtkTypeUInt64*)b)[vtkSortDataArrayComp] ? -1 :
    (((vtkTypeUInt64*)a)[vtkSortDataArrayComp] == ((vtkTypeUInt64*)b)[vtkSortDataArrayComp] ? 0 : 1);
}
#endif // VTK_TYPE_USE___INT64

int vtkSortDataArrayComponentCompare_VTK_ID_TYPE( const void* a, const void* b )
{
  return ((vtkIdType*)a)[vtkSortDataArrayComp] < ((vtkIdType*)b)[vtkSortDataArrayComp] ? -1 :
    (((vtkIdType*)a)[vtkSortDataArrayComp] == ((vtkIdType*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_LONG( const void* a, const void* b )
{
  return ((long*)a)[vtkSortDataArrayComp] < ((long*)b)[vtkSortDataArrayComp] ? -1 :
    (((long*)a)[vtkSortDataArrayComp] == ((long*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_UNSIGNED_LONG( const void* a, const void* b )
{
  return ((unsigned long*)a)[vtkSortDataArrayComp] < ((unsigned long*)b)[vtkSortDataArrayComp] ? -1 :
    (((unsigned long*)a)[vtkSortDataArrayComp] == ((unsigned long*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_INT( const void* a, const void* b )
{
  return ((int*)a)[vtkSortDataArrayComp] < ((int*)b)[vtkSortDataArrayComp] ? -1 :
    (((int*)a)[vtkSortDataArrayComp] == ((int*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_UNSIGNED_INT( const void* a, const void* b )
{
  return ((unsigned int*)a)[vtkSortDataArrayComp] < ((unsigned int*)b)[vtkSortDataArrayComp] ? -1 :
    (((unsigned int*)a)[vtkSortDataArrayComp] == ((unsigned int*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_SHORT( const void* a, const void* b )
{
  return ((short*)a)[vtkSortDataArrayComp] < ((short*)b)[vtkSortDataArrayComp] ? -1 :
    (((short*)a)[vtkSortDataArrayComp] == ((short*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_UNSIGNED_SHORT( const void* a, const void* b )
{
  return ((unsigned short*)a)[vtkSortDataArrayComp] < ((unsigned short*)b)[vtkSortDataArrayComp] ? -1 :
    (((unsigned short*)a)[vtkSortDataArrayComp] == ((unsigned short*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_CHAR( const void* a, const void* b )
{
  return ((char*)a)[vtkSortDataArrayComp] < ((char*)b)[vtkSortDataArrayComp] ? -1 :
    (((char*)a)[vtkSortDataArrayComp] == ((char*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_SIGNED_CHAR( const void* a, const void* b )
{
  return ((signed char*)a)[vtkSortDataArrayComp] < ((signed char*)b)[vtkSortDataArrayComp] ? -1 :
    (((signed char*)a)[vtkSortDataArrayComp] == ((signed char*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_UNSIGNED_CHAR( const void* a, const void* b )
{
  return ((unsigned char*)a)[vtkSortDataArrayComp] < ((unsigned char*)b)[vtkSortDataArrayComp] ? -1 :
    (((unsigned char*)a)[vtkSortDataArrayComp] == ((unsigned char*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_STRING( const void* a, const void* b )
{
  return ((vtkStdString*)a)[vtkSortDataArrayComp] < ((vtkStdString*)b)[vtkSortDataArrayComp] ? -1 :
    (((vtkStdString*)a)[vtkSortDataArrayComp] == ((vtkStdString*)b)[vtkSortDataArrayComp] ? 0 : 1);
}

int vtkSortDataArrayComponentCompare_VTK_VARIANT( const void* a, const void* b )
{
  vtkVariantLessThan comp;
  return comp(((vtkVariant*)a)[vtkSortDataArrayComp], ((vtkVariant*)b)[vtkSortDataArrayComp]) ? -1 :
    (comp(((vtkVariant*)b)[vtkSortDataArrayComp], ((vtkVariant*)a)[vtkSortDataArrayComp]) ? 1 : 0);
}
}

// vtkSortDataArray methods -------------------------------------------------------

void vtkSortDataArray::Sort(vtkIdList *keys)
{
  vtkIdType *data = keys->GetPointer(0);
  vtkIdType numKeys = keys->GetNumberOfIds();
  std::sort(data, data + numKeys);
}

void vtkSortDataArray::Sort(vtkAbstractArray *keys)
{
  if (keys->GetNumberOfComponents() != 1)
    {
    vtkGenericWarningMacro("Can only sort keys that are 1-tuples.");
    return;
    }

  void *data = keys->GetVoidPointer(0);
  vtkIdType numKeys = keys->GetNumberOfTuples();

  switch (keys->GetDataType())
    {
    vtkExtendedTemplateMacro(std::sort(static_cast<VTK_TT *>(data), static_cast<VTK_TT *>(data) + numKeys));
    }
}

void vtkSortDataArray::SortArrayByComponent( vtkAbstractArray* arr, int k )
{
  int nc = arr->GetNumberOfComponents();
  if ( nc <= k )
    {
    vtkGenericWarningMacro( "Cannot sort by column " << k <<
      " since the array only has columns 0 through " << (nc-1) );
    return;
    }

  // This global variable is what makes the method unsafe for threads:
  vtkSortDataArrayComp = k;

  switch(arr->GetDataType())
    {
    case VTK_DOUBLE:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_DOUBLE);
      break;
    case VTK_FLOAT:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_FLOAT);
      break;
#ifdef VTK_TYPE_USE_LONG_LONG
    case VTK_LONG_LONG:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_LONG_LONG);
      break;
    case VTK_UNSIGNED_LONG_LONG:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_UNSIGNED_LONG_LONG);
      break;
#endif // VTK_TYPE_USE_LONG_LONG
#ifdef VTK_TYPE_USE___INT64
    case VTK___INT64:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK___INT64);
      break;
    case VTK_UNSIGNED___INT64:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_UNSIGNED___INT64);
      break;
#endif // VTK_TYPE_USE___INT64
    case VTK_ID_TYPE:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_ID_TYPE);
      break;
    case VTK_LONG:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_LONG);
      break;
    case VTK_UNSIGNED_LONG:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_UNSIGNED_LONG);
      break;
    case VTK_INT:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_INT);
      break;
    case VTK_UNSIGNED_INT:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_UNSIGNED_INT);
      break;
    case VTK_SHORT:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_SHORT);
      break;
    case VTK_UNSIGNED_SHORT:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_UNSIGNED_SHORT);
      break;
    case VTK_CHAR:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_CHAR);
      break;
    case VTK_SIGNED_CHAR:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_SIGNED_CHAR);
      break;
    case VTK_UNSIGNED_CHAR:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_UNSIGNED_CHAR);
      break;
    case VTK_STRING:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_STRING);
      break;
    case VTK_VARIANT:
      qsort(static_cast<void*>(arr->GetVoidPointer(0)),
            static_cast<size_t>(arr->GetNumberOfTuples()),
            static_cast<size_t>(arr->GetDataTypeSize()*nc),
            vtkSortDataArrayComponentCompare_VTK_VARIANT);
      break;
    }
}

void vtkSortDataArray::Sort(vtkIdList *keys, vtkIdList *values)
{
  vtkIdType size = keys->GetNumberOfIds();
  if (size != values->GetNumberOfIds())
    {
    vtkGenericWarningMacro("Cannot sort arrays.  Sizes of keys and values do not agree");
    return;
    }

  vtkSortDataArraySort00(keys->GetPointer(0), values->GetPointer(0), size, 1);
}

void vtkSortDataArray::Sort(vtkIdList *keys, vtkAbstractArray *values)
{
  vtkSortDataArraySort01(keys->GetPointer(0), values, keys->GetNumberOfIds());
}

void vtkSortDataArray::Sort(vtkAbstractArray *keys, vtkIdList *values)
{
  if (keys->GetDataType() == VTK_VARIANT)
    {
    vtkVariantLessThan comp;
    vtkSortDataArraySort00(
      static_cast<vtkVariant*>(keys->GetVoidPointer(0)),
      values->GetPointer(0), values->GetNumberOfIds(), 1, comp);
    }
  else
    {
    vtkSortDataArraySort10(keys, values->GetPointer(0),
                           values->GetNumberOfIds(), 1);
    }
}

void vtkSortDataArray::Sort(vtkAbstractArray *keys, vtkAbstractArray *values)
{
  if (keys->GetDataType() == VTK_VARIANT)
    {
    vtkVariantLessThan comp;
    vtkSortDataArraySort01(
      static_cast<vtkVariant*>(keys->GetVoidPointer(0)), values, keys->GetNumberOfTuples(), comp);
    }
  else
    {
    vtkSortDataArraySort11(keys, values);
    }
}
