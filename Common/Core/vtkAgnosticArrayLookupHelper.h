/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAgnosticArrayLookupHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAgnosticArrayLookupHelper - internal class used by
// vtkAgnosticArray to support LookupValue.
// .SECTION Description

#ifndef vtkAgnosticArrayLookupHelper_h
#define vtkAgnosticArrayLookupHelper_h

#include <algorithm>
#include "vtkIdList.h"

namespace detail
{
  // this can be removed when C++11 is required.
  template< class T > struct remove_const { typedef T type; };
  template< class T > struct remove_const<const T> { typedef T type; };
}

template <class ArrayTypeT>
class vtkAgnosticArrayLookupHelper
{
public:
  typedef ArrayTypeT ArrayType;
  typedef typename ArrayType::ScalarType ScalarType;
  typedef typename ArrayType::TupleIteratorType TupleIteratorType;

  // Constructor.
  vtkAgnosticArrayLookupHelper(ArrayType& associatedArray)
    : AssociatedArray(associatedArray),
    SortedArray(NULL)
    {
    }
  ~vtkAgnosticArrayLookupHelper()
    {
    this->ClearLookup();
    }

  vtkIdType LookupValue(ScalarType elem)
    {
    this->UpdateLookup();
    ValueWithIndex temp;
    temp.Value = elem;
    ValueWithIndex* pos =
      std::lower_bound(this->SortedArray, this->SortedArray + this->SortedArraySize, temp);
    if (pos == (this->SortedArray + this->SortedArraySize))
      {
      return -1;
      }
    if (pos->Value != elem)
      {
      return -1;
      }
    return pos->Index;
    }

  void LookupValue(ScalarType elem, vtkIdList* ids)
    {
    this->UpdateLookup();
    ValueWithIndex temp;
    temp.Value = elem;
    std::pair<ValueWithIndex*, ValueWithIndex*> range =
      std::equal_range(this->SortedArray, this->SortedArray + this->SortedArraySize, temp);
    while (range.first != range.second)
      {
      // assert(range.first->Value == elem);
      ids->InsertNextId(range.first->Index);
      ++range.first;
      }
    }

  // Description:
  // Release any allocated memory for internal data-structures.
  void ClearLookup()
    {
    free(this->SortedArray);
    this->SortedArray = NULL;
    this->SortedArraySize = 0;
    }

private:
  vtkAgnosticArrayLookupHelper(const vtkAgnosticArrayLookupHelper&); // Not implemented.
  void operator=(const vtkAgnosticArrayLookupHelper&); // Not implemented.

  struct ValueWithIndex
    {
    typename detail::remove_const<ScalarType>::type Value;
    vtkIdType Index;
    inline bool operator<(const ValueWithIndex& other) const
      {
      return this->Value < other.Value;
      }
    };

  void UpdateLookup()
    {
    if (this->SortedArray) { return; }

    int numComps = this->AssociatedArray.GetNumberOfComponents();
    this->SortedArraySize = this->AssociatedArray.GetNumberOfTuples() * numComps;

    if (this->SortedArraySize == 0) { return; }

    this->SortedArray = reinterpret_cast<ValueWithIndex*>(malloc(this->SortedArraySize * sizeof(ValueWithIndex)));
    for (TupleIteratorType iter = this->AssociatedArray.Begin(); iter != this->AssociatedArray.End(); ++iter)
      {
      for (int cc=0; cc < numComps; ++cc)
        {
        ValueWithIndex& item = this->SortedArray[iter.GetIndex()*numComps+cc];
        item.Value = iter[cc];
        item.Index = iter.GetIndex();
        // not preserving component index for now.
        }
      }
    std::sort(this->SortedArray, this->SortedArray + this->SortedArraySize);
    }

  ArrayTypeT& AssociatedArray;
  ValueWithIndex* SortedArray;
  vtkIdType SortedArraySize;
};

#endif
