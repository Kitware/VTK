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
#include "vtkIdList.h"

namespace detail
{
  // this can be removed when C++11 is required.
  template< class T > struct remove_const { typedef T type; };
  template< class T > struct remove_const<const T> { typedef T type; };
}

template <class ArrayTypeT>
class vtkGenericDataArrayLookupHelper
{
public:
  typedef ArrayTypeT ArrayType;
  typedef typename ArrayType::ValueType ValueType;

  // Constructor.
  vtkGenericDataArrayLookupHelper()
    : AssociatedArray(NULL),
    SortedArray(NULL)
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
    ValueWithIndex temp;
    temp.Value = elem;
    ValueWithIndex* pos =
      std::lower_bound(this->SortedArray,
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
    this->UpdateLookup();
    ValueWithIndex temp;
    temp.Value = elem;
    std::pair<ValueWithIndex*, ValueWithIndex*> range =
      std::equal_range(this->SortedArray,
                       this->SortedArray + this->SortedArraySize, temp);
    while (range.first != range.second)
    {
      // assert(range.first->Value == elem);
      ids->InsertNextId(range.first->Index);
      ++range.first;
    }
  }

  //@{
  /**
   * Release any allocated memory for internal data-structures.
   */
  void ClearLookup()
  {
    free(this->SortedArray);
    this->SortedArray = NULL;
    this->SortedArraySize = 0;
  }
  //@}

private:
  vtkGenericDataArrayLookupHelper(const vtkGenericDataArrayLookupHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGenericDataArrayLookupHelper&) VTK_DELETE_FUNCTION;

  struct ValueWithIndex
  {
    typename ::detail::remove_const<ValueType>::type Value;
    vtkIdType Index;
    inline bool operator<(const ValueWithIndex& other) const
    {
      return this->Value < other.Value;
    }
  };

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
    std::sort(this->SortedArray, this->SortedArray + this->SortedArraySize);
  }

  ArrayTypeT *AssociatedArray;
  ValueWithIndex* SortedArray;
  vtkIdType SortedArraySize;
};

#endif
// VTK-HeaderTest-Exclude: vtkGenericDataArrayLookupHelper.h
