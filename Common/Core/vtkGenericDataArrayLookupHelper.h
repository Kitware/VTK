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

#include "vtkIdList.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace detail
{
template <typename T, bool>
struct has_NaN;

template <typename T>
struct has_NaN<T, true>
{
  static bool isnan(T x) { return std::isnan(x); }
};

template <typename T>
struct has_NaN<T, false>
{
  static bool isnan(T) { return false; }
};

template <typename T>
bool isnan(T x)
{
  // Select the correct partially specialized type.
  return has_NaN<T, std::numeric_limits<T>::has_quiet_NaN>::isnan(x);
}
} // namespace detail

template <class ArrayTypeT>
class vtkGenericDataArrayLookupHelper
{
public:
  typedef ArrayTypeT ArrayType;
  typedef typename ArrayType::ValueType ValueType;

  vtkGenericDataArrayLookupHelper() = default;

  ~vtkGenericDataArrayLookupHelper() { this->ClearLookup(); }

  void SetArray(ArrayTypeT* array)
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
    auto indices = FindIndexVec(elem);
    if (indices == nullptr)
    {
      return -1;
    }
    return indices->front();
  }

  void LookupValue(ValueType elem, vtkIdList* ids)
  {
    ids->Reset();
    this->UpdateLookup();
    auto indices = FindIndexVec(elem);
    if (indices)
    {
      ids->Allocate(static_cast<vtkIdType>(indices->size()));
      for (auto index : *indices)
      {
        ids->InsertNextId(index);
      }
    }
  }

  //@{
  /**
   * Release any allocated memory for internal data-structures.
   */
  void ClearLookup()
  {
    this->ValueMap.clear();
    this->NanIndices.clear();
  }
  //@}

private:
  vtkGenericDataArrayLookupHelper(const vtkGenericDataArrayLookupHelper&) = delete;
  void operator=(const vtkGenericDataArrayLookupHelper&) = delete;

  void UpdateLookup()
  {
    if (!this->AssociatedArray || (this->AssociatedArray->GetNumberOfTuples() < 1) ||
      (!this->ValueMap.empty() || !this->NanIndices.empty()))
    {
      return;
    }

    vtkIdType num = this->AssociatedArray->GetNumberOfValues();
    this->ValueMap.reserve(num);
    for (vtkIdType i = 0; i < num; ++i)
    {
      auto value = this->AssociatedArray->GetValue(i);
      if (::detail::isnan(value))
      {
        NanIndices.push_back(i);
      }
      this->ValueMap[value].push_back(i);
    }
  }

  // Return a pointer to the relevant vector of indices if specified value was
  // found in the array.
  std::vector<vtkIdType>* FindIndexVec(ValueType value)
  {
    std::vector<vtkIdType>* indices{ nullptr };
    if (::detail::isnan(value) && !this->NanIndices.empty())
    {
      indices = &this->NanIndices;
    }
    const auto& pos = this->ValueMap.find(value);
    if (pos != this->ValueMap.end())
    {
      indices = &pos->second;
    }
    return indices;
  }

  ArrayTypeT* AssociatedArray{ nullptr };
  std::unordered_map<ValueType, std::vector<vtkIdType> > ValueMap;
  std::vector<vtkIdType> NanIndices;
};

#endif
// VTK-HeaderTest-Exclude: vtkGenericDataArrayLookupHelper.h
