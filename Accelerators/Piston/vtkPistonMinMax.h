/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPistonMinMax - computes scalar range on GPU for mapper
// .SECTION Description
// Used in vtkPistonMapper to determine data range as part of color mapping
// process.

#ifndef __vtkPistonMinMax_h
#define __vtkPistonMinMax_h

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/transform_reduce.h>
#include <thrust/functional.h>
#include <thrust/extrema.h>
#include <thrust/random.h>

namespace vtkPiston
{
  // Compute minimum and maximum values in a single reduction

  // minmax_pair stores the minimum and maximum
  // values that have been encountered so far
  template <typename T>
  struct minmax_pair
  {
    T min_val;
    T max_val;
  };

  // minmax_unary_op is a functor that takes in a value x and
  // returns a minmax_pair whose minimum and maximum values
  // are initialized to x.
  template <typename T>
  struct minmax_unary_op
    : public thrust::unary_function< T, minmax_pair<T> >
  {
    __host__ __device__
    minmax_pair<T> operator()(const T& x) const
    {
      minmax_pair<T> result;
      result.min_val = x;
      result.max_val = x;
      return result;
    }
  };

  // minmax_binary_op is a functor that accepts two minmax_pair
  // structs and returns a new minmax_pair whose minimum and
  // maximum values are the min() and max() respectively of
  // the minimums and maximums of the input pairs
  template <typename T>
  struct minmax_binary_op
    : public thrust::binary_function< minmax_pair<T>, minmax_pair<T>, minmax_pair<T> >
  {
    __host__ __device__
    minmax_pair<T> operator()(const minmax_pair<T>& x, const minmax_pair<T>& y) const
    {
      minmax_pair<T> result;
      result.min_val = thrust::min(x.min_val, y.min_val);
      result.max_val = thrust::max(x.max_val, y.max_val);
      return result;
    }
  };

  template <typename T>
  minmax_pair<T> find_min_max(thrust::device_vector<T>* data)
  {
    // Setup arguments
    minmax_unary_op<T>  unary_op;
    minmax_binary_op<T> binary_op;

    // Initialize reduction with the first value
    minmax_pair<T> init = unary_op((*data)[0]);

    // Compute minimum and maximum values
    minmax_pair<T> result = thrust::transform_reduce(
      data->begin(), data->end(), unary_op, init, binary_op);

    return result;
  }
}

#endif // __vtkPistonMinMax_h
// VTK-HeaderTest-Exclude: vtkPistonMinMax.h
