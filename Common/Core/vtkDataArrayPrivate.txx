// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDataArrayPrivate_txx
#define vtkDataArrayPrivate_txx

#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkMathUtilities.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkTypeTraits.h"

#include <algorithm>
#include <array>
#include <vector>

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN

template <typename ArrayT, typename APIType, int RangeNumComps>
class MinAndMax
{
protected:
  ArrayT* Array;
  APIType ReducedRange[2 * RangeNumComps];
  vtkSMPThreadLocal<std::array<APIType, 2 * RangeNumComps>> TLRange;
  const unsigned char* Ghosts;
  unsigned char GhostsToSkip;

public:
  MinAndMax(ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : Array(array)
    , Ghosts(ghosts)
    , GhostsToSkip(ghostsToSkip)
  {
    for (int i = 0, j = 0; i < RangeNumComps; ++i, j += 2)
    {
      this->ReducedRange[j] = vtkTypeTraits<APIType>::Max();
      this->ReducedRange[j + 1] = vtkTypeTraits<APIType>::Min();
    }
  }
  virtual ~MinAndMax() = default;
  virtual void Initialize()
  {
    auto& range = this->TLRange.Local();
    for (int i = 0, j = 0; i < RangeNumComps; ++i, j += 2)
    {
      range[j] = vtkTypeTraits<APIType>::Max();
      range[j + 1] = vtkTypeTraits<APIType>::Min();
    }
  }
  virtual void Reduce()
  {
    for (auto itr = this->TLRange.begin(); itr != this->TLRange.end(); ++itr)
    {
      auto& range = *itr;
      for (int i = 0, j = 0; i < RangeNumComps; ++i, j += 2)
      {
        this->ReducedRange[j] = std::min(this->ReducedRange[j], range[j]);
        this->ReducedRange[j + 1] = std::max(this->ReducedRange[j + 1], range[j + 1]);
      }
    }
  }
  template <typename T>
  void CopyRanges(T* ranges)
  {
    for (int i = 0, j = 0; i < RangeNumComps; ++i, j += 2)
    {
      ranges[j] = static_cast<T>(this->ReducedRange[j]);
      ranges[j + 1] = static_cast<T>(this->ReducedRange[j + 1]);
    }
  }
};

template <int NumComps, typename ArrayT, typename APIType = typename vtk::GetAPIType<ArrayT>>
class AllValuesMinAndMax : public MinAndMax<ArrayT, APIType, NumComps>
{
private:
  using MinAndMaxT = MinAndMax<ArrayT, APIType, NumComps>;

public:
  AllValuesMinAndMax(ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : MinAndMaxT(array, ghosts, ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() override { MinAndMaxT::Initialize(); }
  void Reduce() override { MinAndMaxT::Reduce(); }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto tuples = vtk::DataArrayTupleRange<NumComps>(this->Array, begin, end);
    auto& range = this->TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
      // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
      if (ghostIt && (*(ghostIt++) & this->GhostsToSkip))
      {
        continue;
      }
      for (int i = 0, j = 0; i < NumComps; ++i, j += 2)
      {
        const APIType value = tuple[i];
        vtkMathUtilities::UpdateRange(range[j], range[j + 1], value);
      }
    }
  }
};

template <int NumComps, typename ArrayT, typename APIType = typename vtk::GetAPIType<ArrayT>>
class FiniteMinAndMax : public MinAndMax<ArrayT, APIType, NumComps>
{
private:
  using MinAndMaxT = MinAndMax<ArrayT, APIType, NumComps>;

public:
  FiniteMinAndMax(ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : MinAndMaxT(array, ghosts, ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() override { MinAndMaxT::Initialize(); }
  void Reduce() override { MinAndMaxT::Reduce(); }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto tuples = vtk::DataArrayTupleRange<NumComps>(this->Array, begin, end);
    auto& range = this->TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
      // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
      if (ghostIt && (*(ghostIt++) & this->GhostsToSkip))
      {
        continue;
      }
      for (int i = 0, j = 0; i < NumComps; ++i, j += 2)
      {
        const APIType value = tuple[i];
        vtkMathUtilities::UpdateRangeFinite(range[j], range[j + 1], value);
      }
    }
  }
};

template <int NumComps, typename ArrayT, typename APIType = vtk::GetAPIType<ArrayT>>
class MagnitudeAllValuesMinAndMax : public MinAndMax<ArrayT, APIType, 1>
{
private:
  using MinAndMaxT = MinAndMax<ArrayT, APIType, 1>;

public:
  MagnitudeAllValuesMinAndMax(
    ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : MinAndMaxT(array, ghosts, ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() override { MinAndMaxT::Initialize(); }
  void Reduce() override { MinAndMaxT::Reduce(); }
  template <typename T>
  void CopyRanges(T* ranges)
  {
    MinAndMaxT::CopyRanges(ranges);
    // now that we have computed the smallest and largest value, take the
    // square root of that value.
    ranges[0] = std::sqrt(ranges[0]);
    ranges[1] = std::sqrt(ranges[1]);
  }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto tuples = vtk::DataArrayTupleRange<NumComps>(this->Array, begin, end);
    auto& range = this->TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
      // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
      if (ghostIt && (*(ghostIt++) & this->GhostsToSkip))
      {
        continue;
      }
      APIType squaredSum = 0.0;
      for (int i = 0; i < NumComps; ++i)
      {
        const APIType value = tuple[i];
        squaredSum += value * value;
      }
      vtkMathUtilities::UpdateRange(range[0], range[1], squaredSum);
    }
  }
};

template <int NumComps, typename ArrayT, typename APIType = vtk::GetAPIType<ArrayT>>
class MagnitudeFiniteMinAndMax : public MinAndMax<ArrayT, APIType, 1>
{
private:
  using MinAndMaxT = MinAndMax<ArrayT, APIType, 1>;

public:
  MagnitudeFiniteMinAndMax(ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : MinAndMaxT(array, ghosts, ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() override { MinAndMaxT::Initialize(); }
  void Reduce() override { MinAndMaxT::Reduce(); }
  template <typename T>
  void CopyRanges(T* ranges)
  {
    MinAndMaxT::CopyRanges(ranges);
    // now that we have computed the smallest and largest value, take the
    // square root of that value.
    ranges[0] = std::sqrt(ranges[0]);
    ranges[1] = std::sqrt(ranges[1]);
  }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto tuples = vtk::DataArrayTupleRange<NumComps>(this->Array, begin, end);
    auto& range = this->TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
      // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
      if (ghostIt && (*(ghostIt++) & this->GhostsToSkip))
      {
        continue;
      }
      APIType squaredSum = 0.0;
      for (int i = 0; i < NumComps; ++i)
      {
        const APIType value = tuple[i];
        squaredSum += value * value;
      }
      vtkMathUtilities::UpdateRangeFinite(range[0], range[1], squaredSum);
    }
  }
};

//----------------------------------------------------------------------------
template <int NumComps>
struct ComputeScalarRange
{
  template <class ArrayT, typename RangeValueType>
  bool operator()(ArrayT* array, RangeValueType* ranges, AllValues, const unsigned char* ghosts,
    unsigned char ghostsToSkip)
  {
    AllValuesMinAndMax<NumComps, ArrayT> minmax(array, ghosts, ghostsToSkip);
    vtkSMPTools::For(0, array->GetNumberOfTuples(), minmax);
    minmax.CopyRanges(ranges);
    return true;
  }
  template <class ArrayT, typename RangeValueType>
  bool operator()(ArrayT* array, RangeValueType* ranges, FiniteValues, const unsigned char* ghosts,
    unsigned char ghostsToSkip)
  {
    FiniteMinAndMax<NumComps, ArrayT> minmax(array, ghosts, ghostsToSkip);
    vtkSMPTools::For(0, array->GetNumberOfTuples(), minmax);
    minmax.CopyRanges(ranges);
    return true;
  }
};

//----------------------------------------------------------------------------
template <int NumComps>
struct ComputeVectorRange
{
  template <class ArrayT, typename RangeValueType>
  bool operator()(ArrayT* array, RangeValueType* ranges, AllValues, const unsigned char* ghosts,
    unsigned char ghostsToSkip)
  {
    using APIType =
      std::conditional_t<std::is_floating_point_v<vtk::GetAPIType<ArrayT>>, double, vtkTypeInt64>;
    MagnitudeAllValuesMinAndMax<NumComps, ArrayT, APIType> minmax(array, ghosts, ghostsToSkip);
    vtkSMPTools::For(0, array->GetNumberOfTuples(), minmax);
    minmax.CopyRanges(ranges);
    return true;
  }
  template <class ArrayT, typename RangeValueType>
  bool operator()(ArrayT* array, RangeValueType* ranges, FiniteValues, const unsigned char* ghosts,
    unsigned char ghostsToSkip)
  {
    using APIType =
      std::conditional_t<std::is_floating_point_v<vtk::GetAPIType<ArrayT>>, double, vtkTypeInt64>;
    MagnitudeFiniteMinAndMax<NumComps, ArrayT, APIType> minmax(array, ghosts, ghostsToSkip);
    vtkSMPTools::For(0, array->GetNumberOfTuples(), minmax);
    minmax.CopyRanges(ranges);
    return true;
  }
};

template <typename ArrayT, typename APIType>
class GenericMinAndMax
{
protected:
  ArrayT* Array;
  int RangeNumComps;
  vtkSMPThreadLocal<std::vector<APIType>> TLRange;
  std::vector<APIType> ReducedRange;
  const unsigned char* Ghosts;
  unsigned char GhostsToSkip;

public:
  GenericMinAndMax(
    ArrayT* array, int rangeNumComps, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : Array(array)
    , RangeNumComps(rangeNumComps)
    , ReducedRange(2 * RangeNumComps)
    , Ghosts(ghosts)
    , GhostsToSkip(ghostsToSkip)
  {
    for (int i = 0, j = 0; i < this->RangeNumComps; ++i, j += 2)
    {
      this->ReducedRange[j] = vtkTypeTraits<APIType>::Max();
      this->ReducedRange[j + 1] = vtkTypeTraits<APIType>::Min();
    }
  }
  virtual ~GenericMinAndMax() = default;
  virtual void Initialize()
  {
    auto& range = this->TLRange.Local();
    range.resize(2 * this->RangeNumComps);
    for (int i = 0, j = 0; i < this->RangeNumComps; ++i, j += 2)
    {
      range[j] = vtkTypeTraits<APIType>::Max();
      range[j + 1] = vtkTypeTraits<APIType>::Min();
    }
  }
  virtual void Reduce()
  {
    for (auto itr = this->TLRange.begin(); itr != this->TLRange.end(); ++itr)
    {
      auto& range = *itr;
      for (int i = 0, j = 0; i < this->RangeNumComps; ++i, j += 2)
      {
        this->ReducedRange[j] = std::min(this->ReducedRange[j], range[j]);
        this->ReducedRange[j + 1] = std::max(this->ReducedRange[j + 1], range[j + 1]);
      }
    }
  }
  template <typename T>
  void CopyRanges(T* ranges)
  {
    for (int i = 0, j = 0; i < this->RangeNumComps; ++i, j += 2)
    {
      ranges[j] = static_cast<T>(this->ReducedRange[j]);
      ranges[j + 1] = static_cast<T>(this->ReducedRange[j + 1]);
    }
  }
};

template <typename ArrayT, typename APIType = typename vtk::GetAPIType<ArrayT>>
class AllValuesGenericMinAndMax : public GenericMinAndMax<ArrayT, APIType>
{
private:
  using MinAndMaxT = GenericMinAndMax<ArrayT, APIType>;

public:
  AllValuesGenericMinAndMax(ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : MinAndMaxT(array, array->GetNumberOfComponents(), ghosts, ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() override { MinAndMaxT::Initialize(); }
  void Reduce() override { MinAndMaxT::Reduce(); }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto tuples = vtk::DataArrayTupleRange(this->Array, begin, end);
    auto& range = this->TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
      // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
      if (ghostIt && (*(ghostIt++) & this->GhostsToSkip))
      {
        continue;
      }
      size_t j = 0;
      for (const APIType value : tuple)
      {
        vtkMathUtilities::UpdateRange(range[j], range[j + 1], value);
        j += 2;
      }
    }
  }
};

template <typename ArrayT, typename APIType = typename vtk::GetAPIType<ArrayT>>
class FiniteGenericMinAndMax : public GenericMinAndMax<ArrayT, APIType>
{
private:
  using MinAndMaxT = GenericMinAndMax<ArrayT, APIType>;

public:
  FiniteGenericMinAndMax(ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : MinAndMaxT(array, array->GetNumberOfComponents(), ghosts, ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() override { MinAndMaxT::Initialize(); }
  void Reduce() override { MinAndMaxT::Reduce(); }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto tuples = vtk::DataArrayTupleRange(this->Array, begin, end);
    auto& range = this->TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
      // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
      if (ghostIt && (*(ghostIt++) & this->GhostsToSkip))
      {
        continue;
      }
      size_t j = 0;
      for (const APIType value : tuple)
      {
        vtkMathUtilities::UpdateRangeFinite(range[j], range[j + 1], value);
        j += 2;
      }
    }
  }
};

template <typename ArrayT, typename APIType = vtk::GetAPIType<ArrayT>>
class MagnitudeAllValuesGenericMinAndMax : public GenericMinAndMax<ArrayT, APIType>
{
private:
  using MinAndMaxT = GenericMinAndMax<ArrayT, APIType>;

public:
  MagnitudeAllValuesGenericMinAndMax(
    ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : MinAndMaxT(array, 1, ghosts, ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() override { MinAndMaxT::Initialize(); }
  void Reduce() override { MinAndMaxT::Reduce(); }
  template <typename T>
  void CopyRanges(T* ranges)
  {
    MinAndMaxT::CopyRanges(ranges);
    // now that we have computed the smallest and largest value, take the
    // square root of that value.
    ranges[0] = std::sqrt(ranges[0]);
    ranges[1] = std::sqrt(ranges[1]);
  }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto tuples = vtk::DataArrayTupleRange(this->Array, begin, end);
    auto& range = this->TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
      // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
      if (ghostIt && (*(ghostIt++) & this->GhostsToSkip))
      {
        continue;
      }
      APIType squaredSum = 0.0;
      for (const APIType value : tuple)
      {
        squaredSum += value * value;
      }
      vtkMathUtilities::UpdateRange(range[0], range[1], squaredSum);
    }
  }
};

template <typename ArrayT, typename APIType = vtk::GetAPIType<ArrayT>>
class MagnitudeFiniteGenericMinAndMax : public GenericMinAndMax<ArrayT, APIType>
{
private:
  using MinAndMaxT = GenericMinAndMax<ArrayT, APIType>;

public:
  MagnitudeFiniteGenericMinAndMax(
    ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : MinAndMaxT(array, 1, ghosts, ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() override { MinAndMaxT::Initialize(); }
  void Reduce() override { MinAndMaxT::Reduce(); }
  template <typename T>
  void CopyRanges(T* ranges)
  {
    MinAndMaxT::CopyRanges(ranges);
    // now that we have computed the smallest and largest value, take the
    // square root of that value.
    ranges[0] = std::sqrt(ranges[0]);
    ranges[1] = std::sqrt(ranges[1]);
  }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto tuples = vtk::DataArrayTupleRange(this->Array, begin, end);
    auto& range = this->TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
      // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
      if (ghostIt && (*(ghostIt++) & this->GhostsToSkip))
      {
        continue;
      }
      APIType squaredSum = 0.0;
      for (const APIType value : tuple)
      {
        squaredSum += value * value;
      }
      vtkMathUtilities::UpdateRangeFinite(range[0], range[1], squaredSum);
    }
  }
};

//----------------------------------------------------------------------------
template <class ArrayT, typename RangeValueType>
bool GenericComputeScalarRange(ArrayT* array, RangeValueType* ranges, AllValues,
  const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  AllValuesGenericMinAndMax<ArrayT> minmax(array, ghosts, ghostsToSkip);
  vtkSMPTools::For(0, array->GetNumberOfTuples(), minmax);
  minmax.CopyRanges(ranges);
  return true;
}

//----------------------------------------------------------------------------
template <class ArrayT, typename RangeValueType>
bool GenericComputeScalarRange(ArrayT* array, RangeValueType* ranges, FiniteValues,
  const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  FiniteGenericMinAndMax<ArrayT> minmax(array, ghosts, ghostsToSkip);
  vtkSMPTools::For(0, array->GetNumberOfTuples(), minmax);
  minmax.CopyRanges(ranges);
  return true;
}

//----------------------------------------------------------------------------
template <class ArrayT, typename RangeValueType>
bool GenericComputeVectorRange(ArrayT* array, RangeValueType* ranges, AllValues,
  const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  using APIType =
    std::conditional_t<std::is_floating_point_v<vtk::GetAPIType<ArrayT>>, double, vtkTypeInt64>;
  MagnitudeAllValuesGenericMinAndMax<ArrayT, APIType> minmax(array, ghosts, ghostsToSkip);
  vtkSMPTools::For(0, array->GetNumberOfTuples(), minmax);
  minmax.CopyRanges(ranges);
  return true;
}

//----------------------------------------------------------------------------
template <class ArrayT, typename RangeValueType>
bool GenericComputeVectorRange(ArrayT* array, RangeValueType* ranges, FiniteValues,
  const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  using APIType =
    std::conditional_t<std::is_floating_point_v<vtk::GetAPIType<ArrayT>>, double, vtkTypeInt64>;
  MagnitudeFiniteGenericMinAndMax<ArrayT, APIType> minmax(array, ghosts, ghostsToSkip);
  vtkSMPTools::For(0, array->GetNumberOfTuples(), minmax);
  minmax.CopyRanges(ranges);
  return true;
}

//----------------------------------------------------------------------------
template <typename ArrayT, typename RangeValueType, typename ValueType>
bool DoComputeScalarRange(ArrayT* array, RangeValueType* ranges, ValueType,
  const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  const int numComp = array->GetNumberOfComponents();

  // setup the initial ranges to be the max,min for double
  for (int i = 0, j = 0; i < numComp; ++i, j += 2)
  {
    ranges[j] = vtkTypeTraits<RangeValueType>::Max();
    ranges[j + 1] = vtkTypeTraits<RangeValueType>::Min();
  }

  // do this after we make sure range is max to min
  if (array->GetNumberOfTuples() == 0)
  {
    return false;
  }

  // Special case for single value scalar range. This is done to help the
  // compiler detect it can perform loop optimizations.
  auto tag0 = AllValues{};
  bool result;
  switch (numComp)
  {
    case 1:
      result = ComputeScalarRange<1>()(array, ranges, tag0, ghosts, ghostsToSkip);
      break;
    case 2:
      result = ComputeScalarRange<2>()(array, ranges, tag0, ghosts, ghostsToSkip);
      break;
    case 3:
      result = ComputeScalarRange<3>()(array, ranges, tag0, ghosts, ghostsToSkip);
      break;
    case 4:
      result = ComputeScalarRange<4>()(array, ranges, tag0, ghosts, ghostsToSkip);
      break;
    case 5:
      result = ComputeScalarRange<5>()(array, ranges, tag0, ghosts, ghostsToSkip);
      break;
    case 6:
      result = ComputeScalarRange<6>()(array, ranges, tag0, ghosts, ghostsToSkip);
      break;
    case 7:
      result = ComputeScalarRange<7>()(array, ranges, tag0, ghosts, ghostsToSkip);
      break;
    case 8:
      result = ComputeScalarRange<8>()(array, ranges, tag0, ghosts, ghostsToSkip);
      break;
    case 9:
      result = ComputeScalarRange<9>()(array, ranges, tag0, ghosts, ghostsToSkip);
      break;
    default:
      result = GenericComputeScalarRange(array, ranges, tag0, ghosts, ghostsToSkip);
      break;
  }
  // Attempt to use all values range, if it is valid, because it avoids std::isinf calls
  // and therefore is faster.
  if constexpr (std::is_floating_point_v<RangeValueType> && std::is_same_v<ValueType, FiniteValues>)
  {
    bool allFinite = true;
    for (int i = 0, j = 0; i < numComp; ++i, j += 2)
    {
      if (std::isinf(ranges[j]) || std::isinf(ranges[j + 1]))
      {
        allFinite = false;
        break;
      }
    }
    if (allFinite)
    {
      return result;
    }
    // Special case for single value scalar range. This is done to help the
    // compiler detect it can perform loop optimizations.
    auto tag = FiniteValues{};
    switch (numComp)
    {
      case 1:
        return ComputeScalarRange<1>()(array, ranges, tag, ghosts, ghostsToSkip);
      case 2:
        return ComputeScalarRange<2>()(array, ranges, tag, ghosts, ghostsToSkip);
      case 3:
        return ComputeScalarRange<3>()(array, ranges, tag, ghosts, ghostsToSkip);
      case 4:
        return ComputeScalarRange<4>()(array, ranges, tag, ghosts, ghostsToSkip);
      case 5:
        return ComputeScalarRange<5>()(array, ranges, tag, ghosts, ghostsToSkip);
      case 6:
        return ComputeScalarRange<6>()(array, ranges, tag, ghosts, ghostsToSkip);
      case 7:
        return ComputeScalarRange<7>()(array, ranges, tag, ghosts, ghostsToSkip);
      case 8:
        return ComputeScalarRange<8>()(array, ranges, tag, ghosts, ghostsToSkip);
      case 9:
        return ComputeScalarRange<9>()(array, ranges, tag, ghosts, ghostsToSkip);
      default:
        return GenericComputeScalarRange(array, ranges, tag, ghosts, ghostsToSkip);
    }
  }
  else
  {
    return result;
  }
}

//----------------------------------------------------------------------------
template <typename ArrayT, typename RangeValueType>
bool DoComputeVectorRange(ArrayT* array, RangeValueType range[2], AllValues,
  const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  range[0] = vtkTypeTraits<RangeValueType>::Max();
  range[1] = vtkTypeTraits<RangeValueType>::Min();

  // do this after we make sure range is max to min
  const vtkIdType numTuples = array->GetNumberOfTuples();
  if (numTuples == 0)
  {
    return false;
  }

  // Special case for vector range. This is done to help the
  // compiler detect it can perform loop optimizations.
  auto tag = AllValues{};
  const int numComp = array->GetNumberOfComponents();
  switch (numComp)
  {
    case 1:
      return ComputeVectorRange<1>()(array, range, tag, ghosts, ghostsToSkip);
    case 2:
      return ComputeVectorRange<2>()(array, range, tag, ghosts, ghostsToSkip);
    case 3:
      return ComputeVectorRange<3>()(array, range, tag, ghosts, ghostsToSkip);
    case 4:
      return ComputeVectorRange<4>()(array, range, tag, ghosts, ghostsToSkip);
    case 5:
      return ComputeVectorRange<5>()(array, range, tag, ghosts, ghostsToSkip);
    case 6:
      return ComputeVectorRange<6>()(array, range, tag, ghosts, ghostsToSkip);
    case 7:
      return ComputeVectorRange<7>()(array, range, tag, ghosts, ghostsToSkip);
    case 8:
      return ComputeVectorRange<8>()(array, range, tag, ghosts, ghostsToSkip);
    case 9:
      return ComputeVectorRange<9>()(array, range, tag, ghosts, ghostsToSkip);
    default:
      return GenericComputeVectorRange(array, range, tag, ghosts, ghostsToSkip);
  }
}

//----------------------------------------------------------------------------
template <typename ArrayT, typename RangeValueType>
bool DoComputeVectorRange(ArrayT* array, RangeValueType range[2], FiniteValues,
  const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  const vtkIdType numTuples = array->GetNumberOfTuples();

  range[0] = vtkTypeTraits<RangeValueType>::Max();
  range[1] = vtkTypeTraits<RangeValueType>::Min();

  // do this after we make sure range is max to min
  if (numTuples == 0)
  {
    return false;
  }

  auto allValuesTag = AllValues{};
  const bool result = DoComputeVectorRange(array, range, allValuesTag, ghosts, ghostsToSkip);
  // Attempt to use all values range, if it is valid, because it avoids std::isinf calls
  // and therefore is faster.
  if constexpr (std::is_floating_point_v<RangeValueType>)
  {
    if (!std::isinf(range[0]) && !std::isinf(range[1]))
    {
      return result;
    }
    // Special case for vector range. This is done to help the
    // compiler detect it can perform loop optimizations.
    auto tag = FiniteValues{};
    const int numComp = array->GetNumberOfComponents();
    switch (numComp)
    {
      case 1:
        return ComputeVectorRange<1>()(array, range, tag, ghosts, ghostsToSkip);
      case 2:
        return ComputeVectorRange<2>()(array, range, tag, ghosts, ghostsToSkip);
      case 3:
        return ComputeVectorRange<3>()(array, range, tag, ghosts, ghostsToSkip);
      case 4:
        return ComputeVectorRange<4>()(array, range, tag, ghosts, ghostsToSkip);
      case 5:
        return ComputeVectorRange<5>()(array, range, tag, ghosts, ghostsToSkip);
      case 6:
        return ComputeVectorRange<6>()(array, range, tag, ghosts, ghostsToSkip);
      case 7:
        return ComputeVectorRange<7>()(array, range, tag, ghosts, ghostsToSkip);
      case 8:
        return ComputeVectorRange<8>()(array, range, tag, ghosts, ghostsToSkip);
      case 9:
        return ComputeVectorRange<9>()(array, range, tag, ghosts, ghostsToSkip);
      default:
        return GenericComputeVectorRange(array, range, tag, ghosts, ghostsToSkip);
    }
  }
  else
  {
    return result;
  }
}

VTK_ABI_NAMESPACE_END
} // end namespace vtkDataArrayPrivate

#endif
// VTK-HeaderTest-Exclude: vtkDataArrayPrivate.txx
