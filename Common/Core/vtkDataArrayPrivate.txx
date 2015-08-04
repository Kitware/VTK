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


#include "vtkTypeTraits.h"
#include <algorithm>
#include <cassert> // for assert()

namespace vtkDataArrayPrivate
{
#if defined(_MSC_VER) && ( _MSC_VER < 1900 )
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
#if defined(_MSC_VER) && ( _MSC_VER < 1900 )
using msvc::min;
using msvc::max;
#else
using std::min;
using std::max;
#endif
}

//----------------------------------------------------------------------------
template <class ValueType, int NumComps, int RangeSize>
struct ComputeScalarRange
{
  template<class ArrayT>
  bool operator()(ArrayT *array, double *ranges)
  {
    ValueType tempRange[RangeSize];
    for(int i = 0, j = 0; i < NumComps; ++i, j+=2)
      {
      tempRange[j] = vtkTypeTraits<ValueType>::Max();
      tempRange[j+1] = vtkTypeTraits<ValueType>::Min();
      }

    //compute the range for each component of the data array at the same time
    const int numTuples = array->GetNumberOfTuples();
    for(vtkIdType tupleIdx = 0; tupleIdx < numTuples; ++tupleIdx)
      {
      for(int compIdx = 0, j = 0; compIdx < NumComps; ++compIdx, j+=2)
        {
        tempRange[j] = detail::min(tempRange[j],
          array->GetComponentValue(tupleIdx, compIdx));
        tempRange[j+1] = detail::max(tempRange[j+1],
          array->GetComponentValue(tupleIdx, compIdx));
        }
      }

    //convert the range to doubles
    for (int i = 0, j = 0; i < NumComps; ++i, j+=2)
      {
      ranges[j] = static_cast<double>(tempRange[j]);
      ranges[j+1] = static_cast<double>(tempRange[j+1]);
      }
    return true;
  }
};

//----------------------------------------------------------------------------
template <typename ArrayT>
bool DoComputeScalarRange(ArrayT *array, double *ranges)
{
  typedef typename ArrayT::ValueType ValueType;
  const int numTuples = array->GetNumberOfTuples();
  const int numComp = array->GetNumberOfComponents();

  //setup the initial ranges to be the max,min for double
  for (int i = 0, j = 0; i < numComp; ++i, j+=2)
    {
    ranges[j] =  vtkTypeTraits<double>::Max();
    ranges[j+1] = vtkTypeTraits<double>::Min();
    }

  //do this after we make sure range is max to min
  if (numTuples == 0)
    {
    return false;
    }

  //Special case for single value scalar range. This is done to help the
  //compiler detect it can perform loop optimizations.
  if (numComp == 1)
    {
    return ComputeScalarRange<ValueType,1,2>()(array, ranges);
    }
  else if (numComp == 2)
    {
    return ComputeScalarRange<ValueType,2,4>()(array, ranges);
    }
  else if (numComp == 3)
    {
    return ComputeScalarRange<ValueType,3,6>()(array, ranges);
    }
  else if (numComp == 4)
    {
    return ComputeScalarRange<ValueType,4,8>()(array, ranges);
    }
  else if (numComp == 5)
    {
    return ComputeScalarRange<ValueType,5,10>()(array, ranges);
    }
  else if (numComp == 6)
    {
    return ComputeScalarRange<ValueType,6,12>()(array, ranges);
    }
  else if (numComp == 7)
    {
    return ComputeScalarRange<ValueType,7,14>()(array, ranges);
    }
  else if (numComp == 8)
    {
    return ComputeScalarRange<ValueType,8,16>()(array, ranges);
    }
  else if (numComp == 9)
    {
    return ComputeScalarRange<ValueType,9,18>()(array, ranges);
    }
  else
    {
    //initialize the temp range storage to min/max pairs
    ValueType* tempRange = new ValueType[numComp*2];
    for (int i = 0, j = 0; i < numComp; ++i, j+=2)
      {
      tempRange[j] = vtkTypeTraits<ValueType>::Max();
      tempRange[j+1] = vtkTypeTraits<ValueType>::Min();
      }

    //compute the range for each component of the data array at the same time
    for (vtkIdType tupleIdx; tupleIdx < numTuples; ++tupleIdx)
      {
      for(int compIdx = 0, j = 0; compIdx < numComp; ++compIdx, j+=2)
        {
        tempRange[j] = detail::min(tempRange[j],
          array->GetComponentValue(tupleIdx, compIdx));
        tempRange[j+1] = detail::max(tempRange[j+1],
          array->GetComponentValue(tupleIdx, compIdx));
        }
      }

    //convert the range to doubles
    for (int i = 0, j = 0; i < numComp; ++i, j+=2)
      {
      ranges[j] = static_cast<double>(tempRange[j]);
      ranges[j+1] = static_cast<double>(tempRange[j+1]);
      }

    //cleanup temp range storage
    delete[] tempRange;

    return true;
    }
}

//----------------------------------------------------------------------------
template <typename ArrayT>
bool DoComputeVectorRange(ArrayT *array, double range[2])
{
  const int numTuples = array->GetNumberOfTuples();
  const int numComps = array->GetNumberOfComponents();

  range[0] = vtkTypeTraits<double>::Max();
  range[1] = vtkTypeTraits<double>::Min();

  //do this after we make sure range is max to min
  if (numTuples == 0)
    {
    return false;
    }

  //iterate over all the tuples
  for (vtkIdType tupleIdx; tupleIdx < numTuples; ++tupleIdx)
    {
    double squaredSum = 0.0;
    for (int compIdx = 0; compIdx < numComps; ++compIdx)
      {
      const double t = array->GetComponent(tupleIdx, compIdx);
      squaredSum += t * t;
      }
    range[0] = detail::min(range[0], squaredSum);
    range[1] = detail::max(range[1], squaredSum);
    }

  //now that we have computed the smallest and largest value, take the
  //square root of that value.
  range[0] = sqrt(range[0]);
  range[1] = sqrt(range[1]);

  return true;
}

}
#endif
// VTK-HeaderTest-Exclude: vtkDataArrayPrivate.txx
