/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSort.cxx

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

#include "vtkSort.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkDataArray.h"

#include <vtkstd/algorithm>

// -------------------------------------------------------------------------

vtkStandardNewMacro(vtkSort);
vtkCxxRevisionMacro(vtkSort,"1.1");

vtkSort::vtkSort()
{
}

vtkSort::~vtkSort()
{
}

void vtkSort::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// Sorting templates -------------------------------------------------------

template<class TKey, class TValue>
static inline void swap(TKey *keys, TValue *values, int tupleSize,
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
static void bubble_sort(TKey *keys, TValue *values,
                        vtkIdType size, int tupleSize)
{
  for (int i = 1; i < size; i++)
    {
    for (int j = i; (j > 0) && (keys[j] < keys[j-1]); j--)
      {
      swap(keys, values, tupleSize, j, j-1);
      }
    }
}
template<class TKey, class TValue>
static void quick_sort(TKey *keys, TValue *values,
                       vtkIdType size, int tupleSize)
{
  while (1)
    {
    if (size < 8)
      {
      bubble_sort(keys, values, size, tupleSize);
      return;
      }

    vtkIdType pivot = (vtkIdType)(vtkMath::Random(0, size));
    //vtkIdType pivot = size/2;
    swap(keys, values, tupleSize, 0, pivot);
    // Pivot now stored at index 0.

    vtkIdType left = 1;
    vtkIdType right = size - 1;
    while (1)
      {
      while ((left <= right) && (keys[left] <= keys[0])) left++;
      while ((left <= right) && (keys[right] >= keys[0])) right--;
      if (left > right) break;
      swap(keys, values, tupleSize, left, right);
      }

    // Place the pivot back in the middle
    swap(keys, values, tupleSize, 0, left-1);

    // Recurse
    quick_sort(keys + left, values + left*tupleSize, size-left, tupleSize);
    size = left-1;
    }
}

template<class TKey, class TValue>
static inline void arraysort00(TKey *keys, TValue *values,
                               vtkIdType array_size, int tuple_size)
{
  quick_sort(keys, values, array_size, tuple_size);
}

template<class TKey>
static void arraysort01(TKey *keys, vtkDataArray *values, int array_size)
{
  if (array_size != values->GetNumberOfTuples())
    {
    vtkGenericWarningMacro("Could not sort arrays.  Key and value arrays have different sizes.");
    return;
    }

  switch (values->GetDataType())
    {
    vtkTemplateMacro4(arraysort00, keys, (VTK_TT *)values->GetVoidPointer(0),
                      array_size, values->GetNumberOfComponents());
    }
}

template<class TValue>
static void arraysort10(vtkDataArray *keys, TValue *values,
                        int array_size, int tuple_size)
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
    vtkTemplateMacro4(arraysort00, (VTK_TT*)keys->GetVoidPointer(0), values,
                      array_size, tuple_size);
    }
}

static void arraysort11(vtkDataArray *keys, vtkDataArray *values)
{
  switch (values->GetDataType())
    {
    vtkTemplateMacro4(arraysort10, keys, (VTK_TT *)values->GetVoidPointer(0),
                      values->GetNumberOfTuples(),
                      values->GetNumberOfComponents());
    //Template macro does not contain Id type?
    case VTK_ID_TYPE:
      arraysort10(keys, (vtkIdType *)values->GetVoidPointer(0),
                  values->GetNumberOfTuples(),
                  values->GetNumberOfComponents());
      break;
    }
}

// vtkSort methods -------------------------------------------------------

void vtkSort::Sort(vtkIdList *keys)
{
  vtkIdType *data = keys->GetPointer(0);
  vtkIdType numKeys = keys->GetNumberOfIds();
  vtkstd::sort(data, data + numKeys);
}

// There is no vtkTemplateMacro2, so we hack a macro to make a 2 argument
// function look like a three argument function
#define FUNC2TO3(func, arg1, arg2) func(arg1, arg2)

void vtkSort::Sort(vtkDataArray *keys)
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
    vtkTemplateMacro3(FUNC2TO3,
                      vtkstd::sort, (VTK_TT *)data, ((VTK_TT *)data) + numKeys);
    }
}

void vtkSort::Sort(vtkIdList *keys, vtkIdList *values)
{
  vtkIdType size = keys->GetNumberOfIds();
  if (size != values->GetNumberOfIds())
    {
    vtkGenericWarningMacro("Cannot sort arrays.  Sizes of keys and values do not agree");
    return;
    }

  arraysort00(keys->GetPointer(0), values->GetPointer(0), size, 1);
}

void vtkSort::Sort(vtkIdList *keys, vtkDataArray *values)
{
  arraysort01(keys->GetPointer(0), values, keys->GetNumberOfIds());
}

void vtkSort::Sort(vtkDataArray *keys, vtkIdList *values)
{
  arraysort10(keys, values->GetPointer(0), values->GetNumberOfIds(), 1);
}

void vtkSort::Sort(vtkDataArray *keys, vtkDataArray *values)
{
  arraysort11(keys, values);
}
