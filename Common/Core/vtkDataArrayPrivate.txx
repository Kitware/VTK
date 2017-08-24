/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayPrivate.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkDataArrayPrivate_txx
#define vtkDataArrayPrivate_txx

#include "vtkAssume.h"
#include "vtkDataArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkTypeTraits.h"
#include <algorithm>
#include <array>
#include <cassert> // for assert()
#include <vector>

namespace vtkDataArrayPrivate
{
#if (defined(_MSC_VER) && ( _MSC_VER < 2000 )) || (defined(__INTEL_COMPILER) && ( __INTEL_COMPILER < 1700 ))
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
#if (defined(_MSC_VER) && ( _MSC_VER < 2000 )) || (defined(__INTEL_COMPILER) && ( __INTEL_COMPILER < 1700 ))
using msvc::min;
using msvc::max;
#else
using std::min;
using std::max;
#endif
}

//avoid checking types that don't contain infinity.
namespace detail {
template <typename T, bool> struct has_infinity;

template <typename T>
struct has_infinity<T, true> {
  static bool isinf(T x)
  {
    return std::isinf(x);
  }
};

template <typename T>
struct has_infinity<T, false> { static bool isinf(T) { return false; } };

template <typename T>
bool isinf(T x)
{
  // Select the correct partially specialized type.
  return has_infinity<T, std::numeric_limits<T>::has_infinity>::isinf(x);
}
}

template<typename APIType, int NumComps>
class MinAndMax
{
protected:
  APIType ReducedRange[2 * NumComps];
  vtkSMPThreadLocal<std::array<APIType, 2 * NumComps>> TLRange;
public:
  void Initialize()
  {
    auto &range = this->TLRange.Local();
    for(int i = 0, j = 0; i < NumComps; ++i, j+=2)
    {
      range[j] = vtkTypeTraits<APIType>::Max();
      range[j+1] = vtkTypeTraits<APIType>::Min();
      this->ReducedRange[j] = vtkTypeTraits<APIType>::Max();
      this->ReducedRange[j+1] = vtkTypeTraits<APIType>::Min();
    }
  }
  void Reduce()
  {
    for (auto itr = this->TLRange.begin(); itr != this->TLRange.end(); ++itr)
    {
      auto &range = *itr;
      for(int i = 0, j = 0; i < NumComps; ++i, j+=2)
      {
        this->ReducedRange[j] = detail::min(this->ReducedRange[j], range[j]);
        this->ReducedRange[j+1] = detail::max(this->ReducedRange[j+1], range[j+1]);
      }
    }
  }
  void CopyRanges(double *ranges)
  {
    for (int i = 0, j = 0; i < NumComps; ++i, j+=2)
    {
      ranges[j] = static_cast<double>(this->ReducedRange[j]);
      ranges[j+1] = static_cast<double>(this->ReducedRange[j+1]);
    }
  }
};

template<int NumComps, typename ArrayT, typename APIType = typename vtkDataArrayAccessor<ArrayT>::APIType>
class AllValuesMinAndMax : public MinAndMax<APIType, NumComps>
{
private:
  using MinAndMaxT = MinAndMax<APIType, NumComps>;
  ArrayT *Array;
public:
  AllValuesMinAndMax(ArrayT *array) : MinAndMaxT() , Array(array) {}
  //Help vtkSMPTools find Initialize() and Reduce()
  void Initialize()
  {
    MinAndMaxT::Initialize();
  }
  void Reduce()
  {
    MinAndMaxT::Reduce();
  }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    VTK_ASSUME(this->Array->GetNumberOfComponents() == NumComps);
    vtkDataArrayAccessor<ArrayT> access(this->Array);
    auto &range = MinAndMaxT::TLRange.Local();
    for(vtkIdType tupleIdx = begin; tupleIdx < end; ++tupleIdx)
    {
      for(int compIdx = 0, j = 0; compIdx < NumComps; ++compIdx, j+=2)
      {
        APIType value = access.Get(tupleIdx, compIdx);
        range[j]   = detail::min(range[j], value);
        range[j+1] = detail::max(range[j+1], value);
      }
    }
  }
};

template<int NumComps, typename ArrayT, typename APIType = typename vtkDataArrayAccessor<ArrayT>::APIType>
class FiniteMinAndMax : public MinAndMax<APIType, NumComps>
{
private:
  using MinAndMaxT =  MinAndMax<APIType, NumComps>;
  ArrayT *Array;
public:
  FiniteMinAndMax(ArrayT *array) : MinAndMaxT(), Array(array) {}
  //Help vtkSMPTools find Initialize() and Reduce()
  void Initialize()
  {
    MinAndMaxT::Initialize();
  }
  void Reduce()
  {
    MinAndMaxT::Reduce();
  }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    VTK_ASSUME(this->Array->GetNumberOfComponents() == NumComps);
    vtkDataArrayAccessor<ArrayT> access(this->Array);
    auto &range = MinAndMaxT::TLRange.Local();
    for(vtkIdType tupleIdx = begin; tupleIdx < end; ++tupleIdx)
    {
      for(int compIdx = 0, j = 0; compIdx < NumComps; ++compIdx, j+=2)
      {
        APIType value = access.Get(tupleIdx, compIdx);
        if (!detail::isinf(value))
        {
          range[j]   = detail::min(range[j], value);
          range[j+1] = detail::max(range[j+1], value);
        }
      }
    }
  }
};

template<typename ArrayT, typename APIType = typename vtkDataArrayAccessor<ArrayT>::APIType>
class MagnitudeAllValuesMinAndMax : public MinAndMax<APIType, 1>
{
private:
  using MinAndMaxT =  MinAndMax<APIType, 1>;
  ArrayT *Array;
public:
  MagnitudeAllValuesMinAndMax(ArrayT *array) : MinAndMaxT(), Array(array) {}
  //Help vtkSMPTools find Initialize() and Reduce()
  void Initialize()
  {
    MinAndMaxT::Initialize();
  }
  void Reduce()
  {
    MinAndMaxT::Reduce();
  }
  void CopyRanges(double *ranges)
  {
    MinAndMaxT::CopyRanges(ranges);
    //now that we have computed the smallest and largest value, take the
    //square root of that value.
    ranges[0] = std::sqrt(ranges[0]);
    ranges[1] = std::sqrt(ranges[1]);
  }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const int NumComps = this->Array->GetNumberOfComponents();
    vtkDataArrayAccessor<ArrayT> access(this->Array);
    auto &range = MinAndMaxT::TLRange.Local();
    for(vtkIdType tupleIdx = begin; tupleIdx < end; ++tupleIdx)
    {
      APIType squaredSum = 0.0;
      for (int compIdx = 0; compIdx < NumComps; ++compIdx)
      {
        const APIType t = static_cast<APIType>(access.Get(tupleIdx, compIdx));
        squaredSum += t * t;
      }
      range[0] = detail::min(range[0], squaredSum);
      range[1] = detail::max(range[1], squaredSum);
    }
  }
};


template<typename ArrayT, typename APIType = typename vtkDataArrayAccessor<ArrayT>::APIType>
class MagnitudeFiniteMinAndMax : public MinAndMax<APIType, 1>
{
private:
  using MinAndMaxT =  MinAndMax<APIType, 1>;
  ArrayT *Array;
public:
  MagnitudeFiniteMinAndMax(ArrayT *array) : MinAndMaxT(), Array(array) {}
  //Help vtkSMPTools find Initialize() and Reduce()
  void Initialize()
  {
    MinAndMaxT::Initialize();
  }
  void Reduce()
  {
    MinAndMaxT::Reduce();
  }
  void CopyRanges(double *ranges)
  {
    MinAndMaxT::CopyRanges(ranges);
    //now that we have computed the smallest and largest value, take the
    //square root of that value.
    ranges[0] = std::sqrt(ranges[0]);
    ranges[1] = std::sqrt(ranges[1]);
  }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const int NumComps = this->Array->GetNumberOfComponents();
    vtkDataArrayAccessor<ArrayT> access(this->Array);
    auto &range = MinAndMaxT::TLRange.Local();
    for(vtkIdType tupleIdx = begin; tupleIdx < end; ++tupleIdx)
    {
      APIType squaredSum = 0.0;
      for (int compIdx = 0; compIdx < NumComps; ++compIdx)
      {
        const APIType t = static_cast<APIType>(access.Get(tupleIdx, compIdx));
        squaredSum += t * t;
      }
      if (!detail::isinf(squaredSum))
      {
        range[0] = detail::min(range[0], squaredSum);
        range[1] = detail::max(range[1], squaredSum);
      }
    }
  }
};

struct AllValues {};
struct FiniteValues {};

//----------------------------------------------------------------------------
template <int NumComps>
struct ComputeScalarRange
{
  template<class ArrayT>
  bool operator()(ArrayT *array, double *ranges, AllValues)
  {
    AllValuesMinAndMax<NumComps, ArrayT> minmax(array);
    vtkSMPTools::For(0, array->GetNumberOfTuples(), minmax);
    minmax.CopyRanges(ranges);
    return true;
  }
  template<class ArrayT>
  bool operator()(ArrayT *array, double *ranges, FiniteValues)
  {
    FiniteMinAndMax<NumComps, ArrayT> minmax(array);
    vtkSMPTools::For(0, array->GetNumberOfTuples(), minmax);
    minmax.CopyRanges(ranges);
    return true;
  }
};

template<typename ArrayT, typename APIType>
class GenericMinAndMax
{
protected:
  ArrayT *Array;
  vtkIdType NumComps;
  vtkSMPThreadLocal<std::vector<APIType>> TLRange;
  std::vector<APIType> ReducedRange;
public:
  GenericMinAndMax(ArrayT * array) : Array(array), NumComps(Array->GetNumberOfComponents()), ReducedRange(2 * NumComps) {}
  void Initialize()
  {
    auto &range = this->TLRange.Local();
    range.resize(2 * this->NumComps);
    for(int i = 0, j = 0; i < this->NumComps; ++i, j+=2)
    {
      range[j] = vtkTypeTraits<APIType>::Max();
      range[j+1] = vtkTypeTraits<APIType>::Min();
      this->ReducedRange[j] = vtkTypeTraits<APIType>::Max();
      this->ReducedRange[j+1] = vtkTypeTraits<APIType>::Min();
    }
  }
  void Reduce()
  {
    for (auto itr = this->TLRange.begin(); itr != this->TLRange.end(); ++itr)
    {
      auto &range = *itr;
      for(int i = 0, j = 0; i < this->NumComps; ++i, j+=2)
      {
        this->ReducedRange[j] = detail::min(this->ReducedRange[j], range[j]);
        this->ReducedRange[j+1] = detail::max(this->ReducedRange[j+1], range[j+1]);
      }
    }
  }
  void CopyRanges(double *ranges)
  {
    for (int i = 0, j = 0; i < NumComps; ++i, j+=2)
    {
      ranges[j] = static_cast<double>(this->ReducedRange[j]);
      ranges[j+1] = static_cast<double>(this->ReducedRange[j+1]);
    }
  }
};

template<typename ArrayT, typename APIType = typename vtkDataArrayAccessor<ArrayT>::APIType>
class AllValuesGenericMinAndMax : public GenericMinAndMax<ArrayT, APIType>
{
private:
  using MinAndMaxT =  GenericMinAndMax<ArrayT, APIType>;
public:
  AllValuesGenericMinAndMax(ArrayT *array) : MinAndMaxT(array) {}
  //Help vtkSMPTools find Initialize() and Reduce()
  void Initialize()
  {
    MinAndMaxT::Initialize();
  }
  void Reduce()
  {
    MinAndMaxT::Reduce();
  }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    vtkDataArrayAccessor<ArrayT> access(MinAndMaxT::Array);
    auto &range = MinAndMaxT::TLRange.Local();
    for(vtkIdType tupleIdx = begin; tupleIdx < end; ++tupleIdx)
    {
      for(int compIdx = 0, j = 0; compIdx < MinAndMaxT::NumComps; ++compIdx, j+=2)
      {
        APIType value = access.Get(tupleIdx, compIdx);
        range[j]   = detail::min(range[j], value);
        range[j+1] = detail::max(range[j+1], value);
      }
    }
  }
};

template<typename ArrayT, typename APIType = typename vtkDataArrayAccessor<ArrayT>::APIType>
class FiniteGenericMinAndMax : public GenericMinAndMax<ArrayT,APIType>
{
private:
  using MinAndMaxT =  GenericMinAndMax<ArrayT, APIType>;
public:
  FiniteGenericMinAndMax(ArrayT *array) : MinAndMaxT(array) {}
  //Help vtkSMPTools find Initialize() and Reduce()
  void Initialize()
  {
    MinAndMaxT::Initialize();
  }
  void Reduce()
  {
    MinAndMaxT::Reduce();
  }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    vtkDataArrayAccessor<ArrayT> access(MinAndMaxT::Array);
    auto &range = MinAndMaxT::TLRange.Local();
    for(vtkIdType tupleIdx = begin; tupleIdx < end; ++tupleIdx)
    {
      for(int compIdx = 0, j = 0; compIdx < MinAndMaxT::NumComps; ++compIdx, j+=2)
      {
        APIType value = access.Get(tupleIdx, compIdx);
        if (!detail::isinf(value))
        {
          range[j]   = detail::min(range[j], value);
          range[j+1] = detail::max(range[j+1], value);
        }
      }
    }
  }
};

template<class ArrayT>
bool GenericComputeScalarRange(ArrayT *array, double *ranges, AllValues)
{
  AllValuesGenericMinAndMax<ArrayT> minmax(array);
  vtkSMPTools::For(0,array->GetNumberOfTuples(),minmax);
  minmax.CopyRanges(ranges);
  return true;
}

template<class ArrayT>
bool GenericComputeScalarRange(ArrayT *array, double *ranges, FiniteValues)
{
  FiniteGenericMinAndMax<ArrayT> minmax(array);
  vtkSMPTools::For(0,array->GetNumberOfTuples(),minmax);
  minmax.CopyRanges(ranges);
  return true;
}

//----------------------------------------------------------------------------
template <typename ArrayT, typename ValueType>
bool DoComputeScalarRange(ArrayT *array, double *ranges, ValueType tag)
{
  vtkDataArrayAccessor<ArrayT> access(array);
  const int numComp = array->GetNumberOfComponents();

  //setup the initial ranges to be the max,min for double
  for (int i = 0, j = 0; i < numComp; ++i, j+=2)
  {
    ranges[j] =  vtkTypeTraits<double>::Max();
    ranges[j+1] = vtkTypeTraits<double>::Min();
  }

  //do this after we make sure range is max to min
  if (array->GetNumberOfTuples() == 0)
  {
    return false;
  }

  //Special case for single value scalar range. This is done to help the
  //compiler detect it can perform loop optimizations.
  if (numComp == 1)
  {
    return ComputeScalarRange<1>()(array, ranges, tag);
  }
  else if (numComp == 2)
  {
    return ComputeScalarRange<2>()(array, ranges, tag);
  }
  else if (numComp == 3)
  {
    return ComputeScalarRange<3>()(array, ranges, tag);
  }
  else if (numComp == 4)
  {
    return ComputeScalarRange<4>()(array, ranges, tag);
  }
  else if (numComp == 5)
  {
    return ComputeScalarRange<5>()(array, ranges, tag);
  }
  else if (numComp == 6)
  {
    return ComputeScalarRange<6>()(array, ranges, tag);
  }
  else if (numComp == 7)
  {
    return ComputeScalarRange<7>()(array, ranges, tag);
  }
  else if (numComp == 8)
  {
    return ComputeScalarRange<8>()(array, ranges, tag);
  }
  else if (numComp == 9)
  {
    return ComputeScalarRange<9>()(array, ranges, tag);
  }
  else
  {
    return GenericComputeScalarRange(array, ranges, tag);
  }
}

//----------------------------------------------------------------------------
template <typename ArrayT>
bool DoComputeVectorRange(ArrayT *array, double range[2], AllValues)
{
  const vtkIdType numTuples = array->GetNumberOfTuples();
  range[0] = vtkTypeTraits<double>::Max();
  range[1] = vtkTypeTraits<double>::Min();

  //do this after we make sure range is max to min
  if (numTuples == 0)
  {
    return false;
  }

  MagnitudeAllValuesMinAndMax<ArrayT, double> MinAndMax(array);
  vtkSMPTools::For(0, numTuples, MinAndMax);
  MinAndMax.CopyRanges(range);
  return true;
}

//----------------------------------------------------------------------------
template <typename ArrayT>
bool DoComputeVectorRange(ArrayT *array, double range[2], FiniteValues)
{
  const vtkIdType numTuples = array->GetNumberOfTuples();

  range[0] = vtkTypeTraits<double>::Max();
  range[1] = vtkTypeTraits<double>::Min();

  //do this after we make sure range is max to min
  if (numTuples == 0)
  {
    return false;
  }

  MagnitudeFiniteMinAndMax<ArrayT, double> MinAndMax(array);
  vtkSMPTools::For(0, numTuples, MinAndMax);
  MinAndMax.CopyRanges(range);
  return true;
}

} // end namespace vtkDataArrayPrivate
#endif
// VTK-HeaderTest-Exclude: vtkDataArrayPrivate.txx
