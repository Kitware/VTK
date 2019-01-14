/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataArrayLookupHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGenericDataArrayLookupHelper
 * @brief   internal class used by
 * vtkGenericDataArray to support LookupValue.
 *
*/

#ifndef vtkGenericDataArrayLookupHelper_h
#define vtkGenericDataArrayLookupHelper_h

#include <algorithm>
#include <cmath>
#include "vtkIdList.h"

namespace detail
{
// this can be removed when C++11 is required.
template< class T > struct remove_const { typedef T type; };
template< class T > struct remove_const<const T> { typedef T type; };

template <typename T, bool> struct has_NaN;

template <typename T>
struct has_NaN<T, true>
{
static bool isnan(T x)
{
  return std::isnan(x);
}
};

template <typename T>
struct has_NaN<T, false>
{
  static bool isnan(T)
  {
    return false;
  }
};

template <typename T>
bool isnan(T x)
{
  // Select the correct partially specialized type.
  return has_NaN<T, std::numeric_limits<T>::has_quiet_NaN>::isnan(x);
}
}

template <class ArrayTypeT>
class vtkGenericDataArrayLookupHelper
{
public:
  typedef ArrayTypeT ArrayType;
  typedef typename ArrayType::ValueType ValueType;

  // Constructor.
  vtkGenericDataArrayLookupHelper()
    : AssociatedArray(nullptr), SortedArray(nullptr),
    FirstValue(nullptr), SortedArraySize(0)
  {
  }
  ~vtkGenericDataArrayLookupHelper()
  {
    this->ClearLookup();
  }

  void SetArray(ArrayTypeT *array)
  {
    if (this->AssociatedArray != array)
    {
      this->ClearLookup();
      this->AssociatedArray = array;
    }
  }

  vtkIdType LookupValue(ValueType elem)
  {
    this->UpdateLookup();

    if (this->SortedArraySize == 0)
    {
      return -1;
    }

    if(::detail::isnan(elem))
    {
      if(this->SortedArray && ::detail::isnan(this->SortedArray->Value))
      {
        return this->SortedArray->Index;
      }
      else
      {
        return -1;
      }
    }

    ValueWithIndex temp;
    temp.Value = elem;
    ValueWithIndex* pos =
      std::lower_bound(this->FirstValue,
                     this->SortedArray + this->SortedArraySize, temp);
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

  void LookupValue(ValueType elem, vtkIdList* ids)
  {
    ids->Reset();
    this->UpdateLookup();

    if (this->SortedArraySize == 0)
    {
     return;
    }

    if(::detail::isnan(elem))
    {
      ValueWithIndex *range = this->SortedArray;
      while (range != this->FirstValue)
      {
        ids->InsertNextId(range->Index);
        ++range;
      }
    }
    else
    {
      ValueWithIndex temp;
      temp.Value = elem;
      std::pair<ValueWithIndex*, ValueWithIndex*> range =
        std::equal_range(this->FirstValue,
                         this->SortedArray + this->SortedArraySize, temp);
      while (range.first != range.second)
      {
        // assert(range.first->Value == elem);
        ids->InsertNextId(range.first->Index);
        ++range.first;
      }
    }
  }

  //@{
  /**
   * Release any allocated memory for internal data-structures.
   */
  void ClearLookup()
  {
    free(this->SortedArray);
    this->SortedArray = nullptr;
    this->SortedArraySize = 0;
  }
  //@}

private:
  vtkGenericDataArrayLookupHelper(const vtkGenericDataArrayLookupHelper&) = delete;
  void operator=(const vtkGenericDataArrayLookupHelper&) = delete;

  struct ValueWithIndex
  {
    typename ::detail::remove_const<ValueType>::type Value;
    vtkIdType Index;
    inline bool operator<(const ValueWithIndex& other) const
    {
      return this->Value < other.Value;
    }
  };

  static bool isnan(const ValueWithIndex &tmp)
  {
    return ::detail::isnan(tmp.Value);
  }

  void UpdateLookup()
  {
    if (!this->AssociatedArray || this->SortedArray)
    {
      return;
    }

    int numComps = this->AssociatedArray->GetNumberOfComponents();
    this->SortedArraySize =
        this->AssociatedArray->GetNumberOfTuples() * numComps;

    if (this->SortedArraySize == 0)
    {
      return;
    }

    this->SortedArray = reinterpret_cast<ValueWithIndex*>(
          malloc(this->SortedArraySize * sizeof(ValueWithIndex)));
    for (vtkIdType cc = 0, max = this->AssociatedArray->GetNumberOfValues();
         cc < max; ++cc)
    {
      ValueWithIndex& item = this->SortedArray[cc];
      item.Value = this->AssociatedArray->GetValue(cc);
      item.Index = cc;
    }
    this->FirstValue = std::partition(this->SortedArray, this->SortedArray + this->SortedArraySize, isnan);
    std::sort(this->FirstValue, this->SortedArray + this->SortedArraySize);
  }

  ArrayTypeT *AssociatedArray;
  ValueWithIndex* SortedArray;
  ValueWithIndex* FirstValue;
  vtkIdType SortedArraySize;
};

#endif
// VTK-HeaderTest-Exclude: vtkGenericDataArrayLookupHelper.h
