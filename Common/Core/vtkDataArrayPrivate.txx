// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDataArrayPrivate_txx
#define vtkDataArrayPrivate_txx

#ifndef VTK_GDA_TEMPLATE_EXTERN

#include "vtkAssume.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkMathUtilities.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkTypeTraits.h"

#include <algorithm>
#include <array>
#include <cassert> // for assert()
#include <limits>
#include <vector>

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN
#if (defined(_MSC_VER) && (_MSC_VER < 2000)) ||                                                    \
  (defined(__INTEL_COMPILER) && (__INTEL_COMPILER < 1700))
namespace msvc
{
//----------------------------------------------------------------------------
// Those min and max functions replace std ones because their
// implementation used to generate very slow code with MSVC.
// See https://randomascii.wordpress.com/2013/11/24/stdmin-causing-three-times-slowdown-on-vc/
// The comparison expression in min/max are written so that if the "condition" is false,
// the "left" value is returned. This is consistent with STL's implementations
// and also handles the cases where the right value may be a NaN properly.
// All code using these methods should ensure that the "left" value is never
// NaN.
// We use _MSC_VER < 2000 instead of 1900 not due to performance issues, but
// because MSVC 2015 (_MSC_VER=1900) doesn't handle NaNs properly in optimized
// builds.
// icpc version 16 also doesn't handle NaNs properly.
// The order is correct in icpc version 17.
template <class ValueType>
ValueType max(const ValueType& left, const ValueType& right)
{
  return right > left ? right : left;
}

template <class ValueType>
ValueType min(const ValueType& left, const ValueType& right)
{
  return right <= left ? right : left;
}
}
#endif

namespace detail
{
#if (defined(_MSC_VER) && (_MSC_VER < 2000)) ||                                                    \
  (defined(__INTEL_COMPILER) && (__INTEL_COMPILER < 1700))
using msvc::max;
using msvc::min;
#else
using std::max;
using std::min;
#endif
}

// avoid checking types that don't contain infinity.
namespace detail
{
template <typename T, bool>
struct has_infinity;

template <typename T>
struct has_infinity<T, true>
{
  static bool isinf(T x) { return std::isinf(x); }
};

template <typename T>
struct has_infinity<T, false>
{
  static bool isinf(T) { return false; }
};

template <typename T>
bool isinf(T x)
{
  // Select the correct partially specialized type.
  return has_infinity<T, std::numeric_limits<T>::has_infinity>::isinf(x);
}
}

template <typename APIType, int NumComps>
class MinAndMax
{
protected:
  APIType ReducedRange[2 * NumComps];
  vtkSMPThreadLocal<std::array<APIType, 2 * NumComps>> TLRange;

public:
  MinAndMax()
  {
    for (int i = 0, j = 0; i < NumComps; ++i, j += 2)
    {
      this->ReducedRange[j] = vtkTypeTraits<APIType>::Max();
      this->ReducedRange[j + 1] = vtkTypeTraits<APIType>::Min();
    }
  }
  void Initialize()
  {
    auto& range = this->TLRange.Local();
    for (int i = 0, j = 0; i < NumComps; ++i, j += 2)
    {
      range[j] = vtkTypeTraits<APIType>::Max();
      range[j + 1] = vtkTypeTraits<APIType>::Min();
    }
  }
  void Reduce()
  {
    for (auto itr = this->TLRange.begin(); itr != this->TLRange.end(); ++itr)
    {
      auto& range = *itr;
      for (int i = 0, j = 0; i < NumComps; ++i, j += 2)
      {
        this->ReducedRange[j] = detail::min(this->ReducedRange[j], range[j]);
        this->ReducedRange[j + 1] = detail::max(this->ReducedRange[j + 1], range[j + 1]);
      }
    }
  }
  template <typename T>
  void CopyRanges(T* ranges)
  {
    for (int i = 0, j = 0; i < NumComps; ++i, j += 2)
    {
      ranges[j] = static_cast<T>(this->ReducedRange[j]);
      ranges[j + 1] = static_cast<T>(this->ReducedRange[j + 1]);
    }
  }
};

template <int NumComps, typename ArrayT, typename APIType = typename vtk::GetAPIType<ArrayT>>
class AllValuesMinAndMax : public MinAndMax<APIType, NumComps>
{
private:
  using MinAndMaxT = MinAndMax<APIType, NumComps>;
  ArrayT* Array;
  const unsigned char* Ghosts;
  unsigned char GhostsToSkip;

public:
  AllValuesMinAndMax(ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : MinAndMaxT()
    , Array(array)
    , Ghosts(ghosts)
    , GhostsToSkip(ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() { MinAndMaxT::Initialize(); }
  void Reduce() { MinAndMaxT::Reduce(); }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto tuples = vtk::DataArrayTupleRange<NumComps>(this->Array, begin, end);
    auto& range = MinAndMaxT::TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
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

template <int NumComps, typename ArrayT, typename APIType = typename vtk::GetAPIType<ArrayT>>
class FiniteMinAndMax : public MinAndMax<APIType, NumComps>
{
private:
  using MinAndMaxT = MinAndMax<APIType, NumComps>;
  ArrayT* Array;
  const unsigned char* Ghosts;
  unsigned char GhostsToSkip;

public:
  FiniteMinAndMax(ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : MinAndMaxT()
    , Array(array)
    , Ghosts(ghosts)
    , GhostsToSkip(ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() { MinAndMaxT::Initialize(); }
  void Reduce() { MinAndMaxT::Reduce(); }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto tuples = vtk::DataArrayTupleRange<NumComps>(this->Array, begin, end);
    auto& range = MinAndMaxT::TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
      if (ghostIt && (*(ghostIt++) & this->GhostsToSkip))
      {
        continue;
      }
      size_t j = 0;
      for (const APIType value : tuple)
      {
        if (!detail::isinf(value))
        {
          vtkMathUtilities::UpdateRange(range[j], range[j + 1], value);
        }
        j += 2;
      }
    }
  }
};

template <typename ArrayT, typename APIType = typename vtk::GetAPIType<ArrayT>>
class MagnitudeAllValuesMinAndMax : public MinAndMax<APIType, 1>
{
private:
  using MinAndMaxT = MinAndMax<APIType, 1>;
  ArrayT* Array;
  const unsigned char* Ghosts;
  unsigned char GhostsToSkip;

public:
  MagnitudeAllValuesMinAndMax(
    ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : MinAndMaxT()
    , Array(array)
    , Ghosts(ghosts)
    , GhostsToSkip(ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() { MinAndMaxT::Initialize(); }
  void Reduce() { MinAndMaxT::Reduce(); }
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
    auto& range = MinAndMaxT::TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
      if (ghostIt && (*(ghostIt++) & this->GhostsToSkip))
      {
        continue;
      }
      APIType squaredSum = 0.0;
      for (const APIType value : tuple)
      {
        squaredSum += value * value;
      }
      range[0] = detail::min(range[0], squaredSum);
      range[1] = detail::max(range[1], squaredSum);
    }
  }
};

template <typename ArrayT, typename APIType = typename vtk::GetAPIType<ArrayT>>
class MagnitudeFiniteMinAndMax : public MinAndMax<APIType, 1>
{
private:
  using MinAndMaxT = MinAndMax<APIType, 1>;
  ArrayT* Array;
  const unsigned char* Ghosts;
  unsigned char GhostsToSkip;
  unsigned char GhsotsToKeep;

public:
  MagnitudeFiniteMinAndMax(ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : MinAndMaxT()
    , Array(array)
    , Ghosts(ghosts)
    , GhostsToSkip(ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() { MinAndMaxT::Initialize(); }
  void Reduce() { MinAndMaxT::Reduce(); }
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
    auto& range = MinAndMaxT::TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
      if (ghostIt && (*(ghostIt++) & this->GhostsToSkip))
      {
        continue;
      }
      APIType squaredSum = 0.0;
      for (const APIType value : tuple)
      {
        squaredSum += value * value;
      }
      if (!detail::isinf(squaredSum))
      {
        range[0] = detail::min(range[0], squaredSum);
        range[1] = detail::max(range[1], squaredSum);
      }
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

template <typename ArrayT, typename APIType>
class GenericMinAndMax
{
protected:
  ArrayT* Array;
  vtkIdType NumComps;
  vtkSMPThreadLocal<std::vector<APIType>> TLRange;
  std::vector<APIType> ReducedRange;
  const unsigned char* Ghosts;
  unsigned char GhostsToSkip;

public:
  GenericMinAndMax(ArrayT* array, const unsigned char* ghosts, unsigned char ghostsToSkip)
    : Array(array)
    , NumComps(Array->GetNumberOfComponents())
    , ReducedRange(2 * NumComps)
    , Ghosts(ghosts)
    , GhostsToSkip(ghostsToSkip)
  {
    for (int i = 0, j = 0; i < this->NumComps; ++i, j += 2)
    {
      this->ReducedRange[j] = vtkTypeTraits<APIType>::Max();
      this->ReducedRange[j + 1] = vtkTypeTraits<APIType>::Min();
    }
  }
  void Initialize()
  {
    auto& range = this->TLRange.Local();
    range.resize(2 * this->NumComps);
    for (int i = 0, j = 0; i < this->NumComps; ++i, j += 2)
    {
      range[j] = vtkTypeTraits<APIType>::Max();
      range[j + 1] = vtkTypeTraits<APIType>::Min();
    }
  }
  void Reduce()
  {
    for (auto itr = this->TLRange.begin(); itr != this->TLRange.end(); ++itr)
    {
      auto& range = *itr;
      for (int i = 0, j = 0; i < this->NumComps; ++i, j += 2)
      {
        this->ReducedRange[j] = detail::min(this->ReducedRange[j], range[j]);
        this->ReducedRange[j + 1] = detail::max(this->ReducedRange[j + 1], range[j + 1]);
      }
    }
  }
  template <typename T>
  void CopyRanges(T* ranges)
  {
    for (int i = 0, j = 0; i < NumComps; ++i, j += 2)
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
    : MinAndMaxT(array, ghosts, ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() { MinAndMaxT::Initialize(); }
  void Reduce() { MinAndMaxT::Reduce(); }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto tuples = vtk::DataArrayTupleRange(this->Array, begin, end);
    auto& range = MinAndMaxT::TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
      if (ghostIt && (*(ghostIt++) & this->GhostsToSkip))
      {
        continue;
      }
      size_t j = 0;
      for (const APIType value : tuple)
      {
        range[j] = detail::min(range[j], value);
        range[j + 1] = detail::max(range[j + 1], value);
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
    : MinAndMaxT(array, ghosts, ghostsToSkip)
  {
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize() { MinAndMaxT::Initialize(); }
  void Reduce() { MinAndMaxT::Reduce(); }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto tuples = vtk::DataArrayTupleRange(this->Array, begin, end);
    auto& range = MinAndMaxT::TLRange.Local();
    const unsigned char* ghostIt = this->Ghosts ? this->Ghosts + begin : nullptr;
    for (const auto tuple : tuples)
    {
      if (ghostIt && (*(ghostIt++) & this->GhostsToSkip))
      {
        continue;
      }
      size_t j = 0;
      for (const APIType value : tuple)
      {
        if (!detail::isinf(value))
        {
          range[j] = detail::min(range[j], value);
          range[j + 1] = detail::max(range[j + 1], value);
        }
        j += 2;
      }
    }
  }
};

template <class ArrayT, typename RangeValueType>
bool GenericComputeScalarRange(ArrayT* array, RangeValueType* ranges, AllValues,
  const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  AllValuesGenericMinAndMax<ArrayT> minmax(array, ghosts, ghostsToSkip);
  vtkSMPTools::For(0, array->GetNumberOfTuples(), minmax);
  minmax.CopyRanges(ranges);
  return true;
}

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
template <typename ArrayT, typename RangeValueType, typename ValueType>
bool DoComputeScalarRange(ArrayT* array, RangeValueType* ranges, ValueType tag,
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
  if (numComp == 1)
  {
    return ComputeScalarRange<1>()(array, ranges, tag, ghosts, ghostsToSkip);
  }
  else if (numComp == 2)
  {
    return ComputeScalarRange<2>()(array, ranges, tag, ghosts, ghostsToSkip);
  }
  else if (numComp == 3)
  {
    return ComputeScalarRange<3>()(array, ranges, tag, ghosts, ghostsToSkip);
  }
  else if (numComp == 4)
  {
    return ComputeScalarRange<4>()(array, ranges, tag, ghosts, ghostsToSkip);
  }
  else if (numComp == 5)
  {
    return ComputeScalarRange<5>()(array, ranges, tag, ghosts, ghostsToSkip);
  }
  else if (numComp == 6)
  {
    return ComputeScalarRange<6>()(array, ranges, tag, ghosts, ghostsToSkip);
  }
  else if (numComp == 7)
  {
    return ComputeScalarRange<7>()(array, ranges, tag, ghosts, ghostsToSkip);
  }
  else if (numComp == 8)
  {
    return ComputeScalarRange<8>()(array, ranges, tag, ghosts, ghostsToSkip);
  }
  else if (numComp == 9)
  {
    return ComputeScalarRange<9>()(array, ranges, tag, ghosts, ghostsToSkip);
  }
  else
  {
    return GenericComputeScalarRange(array, ranges, tag, ghosts, ghostsToSkip);
  }
}

//----------------------------------------------------------------------------
// generic implementation that operates on ValueType.
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

  // Always compute at double precision for vector magnitudes. This will
  // give precision errors on large 64-bit ints, but magnitudes aren't usually
  // computed for those.
  MagnitudeAllValuesMinAndMax<ArrayT, double> MinAndMax(array, ghosts, ghostsToSkip);
  vtkSMPTools::For(0, numTuples, MinAndMax);
  MinAndMax.CopyRanges(range);
  return true;
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

  // Always compute at double precision for vector magnitudes. This will
  // give precision errors on large 64-bit ints, but magnitudes aren't usually
  // computed for those.
  MagnitudeFiniteMinAndMax<ArrayT, double> MinAndMax(array, ghosts, ghostsToSkip);
  vtkSMPTools::For(0, numTuples, MinAndMax);
  MinAndMax.CopyRanges(range);
  return true;
}

VTK_ABI_NAMESPACE_END
} // end namespace vtkDataArrayPrivate
#endif // VTK_GDA_TEMPLATE_EXTERN
#endif
// VTK-HeaderTest-Exclude: vtkDataArrayPrivate.txx
