// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include <limits>
#include <unordered_map>
#include <vector>

namespace vtkGenericDataArrayLookupHelper_detail
{
VTK_ABI_NAMESPACE_BEGIN
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
VTK_ABI_NAMESPACE_END
} // namespace detail

VTK_ABI_NAMESPACE_BEGIN
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

  ///@{
  /**
   * Release any allocated memory for internal data-structures.
   */
  void ClearLookup()
  {
    this->ValueMap.clear();
    this->NanIndices.clear();
  }
  ///@}

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
      if (vtkGenericDataArrayLookupHelper_detail::isnan(value))
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
    if (vtkGenericDataArrayLookupHelper_detail::isnan(value) && !this->NanIndices.empty())
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
  std::unordered_map<ValueType, std::vector<vtkIdType>> ValueMap;
  std::vector<vtkIdType> NanIndices;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkGenericDataArrayLookupHelper.h
