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
template <class ValueType>
ValueType max(const ValueType& left, const ValueType& right)
{
  return left > right ? left : right;
}

template <class ValueType>
ValueType min(const ValueType& left, const ValueType& right)
{
  return left <= right ? left : right;
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
  template<class InputIteratorType>
  bool operator()(InputIteratorType begin, InputIteratorType end,
                  double* ranges)
  {
    ValueType tempRange[RangeSize];
    for(int i = 0, j = 0; i < NumComps; ++i, j+=2)
      {
      tempRange[j] = vtkTypeTraits<ValueType>::Max();
      tempRange[j+1] = vtkTypeTraits<ValueType>::Min();
      }

    //compute the range for each component of the data array at the same time
    for(InputIteratorType value = begin; value != end; value+=NumComps)
      {
      for(int i = 0, j = 0; i < NumComps; ++i, j+=2)
        {
        tempRange[j] = detail::min(value[i], tempRange[j]);
        tempRange[j+1] = detail::max(value[i], tempRange[j+1]);
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
template <class ValueType, class InputIteratorType>
bool DoComputeScalarRange(InputIteratorType begin, InputIteratorType end,
                        const int numComp, double* ranges)
{
  //setup the initial ranges to be the max,min for double
  for (int i = 0, j = 0; i < numComp; ++i, j+=2)
    {
    ranges[j] =  vtkTypeTraits<double>::Max();
    ranges[j+1] = vtkTypeTraits<double>::Min();
    }

  //do this after we make sure range is max to min
  if (begin == end)
    {
    return false;
    }

  //verify that length of the array is divisible by the number of components
  //this will make sure we don't walk off the end
  assert((end-begin) % numComp == 0);

  //Special case for single value scalar range. This is done to help the
  //compiler detect it can perform loop optimizations.
  if (numComp == 1)
    {
    return ComputeScalarRange<ValueType,1,2>()(begin, end, ranges);
    }
  else if (numComp == 2)
    {
    return ComputeScalarRange<ValueType,2,4>()(begin, end, ranges);
    }
  else if (numComp == 3)
    {
    return ComputeScalarRange<ValueType,3,6>()(begin, end, ranges);
    }
  else if (numComp == 4)
    {
    return ComputeScalarRange<ValueType,4,8>()(begin, end, ranges);
    }
  else if (numComp == 5)
    {
    return ComputeScalarRange<ValueType,5,10>()(begin, end, ranges);
    }
  else if (numComp == 6)
    {
    return ComputeScalarRange<ValueType,6,12>()(begin, end, ranges);
    }
  else if (numComp == 7)
    {
    return ComputeScalarRange<ValueType,7,14>()(begin, end, ranges);
    }
  else if (numComp == 8)
    {
    return ComputeScalarRange<ValueType,8,16>()(begin, end, ranges);
    }
  else if (numComp == 9)
    {
    return ComputeScalarRange<ValueType,9,18>()(begin, end, ranges);
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
    for (InputIteratorType value = begin; value != end; value+=numComp)
      {
      for(int i = 0, j = 0; i < numComp; ++i, j+=2)
        {
        tempRange[j] = detail::min(value[i], tempRange[j]);
        tempRange[j+1] = detail::max(value[i], tempRange[j+1]);
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
template <class ValueType, class InputIteratorType>
bool DoComputeVectorRange(InputIteratorType begin, InputIteratorType end,
                          int numComp, double range[2])
{
  range[0] = vtkTypeTraits<double>::Max();
  range[1] = vtkTypeTraits<double>::Min();

  //do this after we make sure range is max to min
  if (begin == end)
    {
    return false;
    }

  //verify that length of the array is divisible by the number of components
  //this will make sure we don't walk off the end
  assert((end-begin) % numComp == 0);

  //iterate over all the tuples
  for (InputIteratorType value = begin; value != end; value+=numComp)
    {
    double squaredSum = 0.0;
    for (int i = 0; i < numComp; ++i)
      {
      const double t = static_cast<double>(value[i]);
      squaredSum += t * t;
      }
    range[0] = detail::min(squaredSum, range[0]);
    range[1] = detail::max(squaredSum, range[1]);
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
