// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkArrayInterpolate
 *
 * Computes the weighted sum of a collection of slices from a source
 * array, and stores the results in a slice of a target array.  Note that
 * the number of source slices and weights must match, and the extents of
 * each source slice must match the extents of the target slice.
 *
 * Note: The implementation assumes that operator*(T, double) is defined,
 * and that there is an implicit conversion from its result back to T.
 *
 * If you need to interpolate arrays of T other than double, you will
 * likely want to create your own specialization of this function.
 *
 * The implementation should produce correct results for dense and sparse
 * arrays, but may perform poorly on sparse.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National
 * Laboratories.
 */

#ifndef vtkArrayInterpolate_h
#define vtkArrayInterpolate_h

#include "vtkTypedArray.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkArrayExtents;
class vtkArraySlices;
class vtkArrayWeights;

//

template <typename T>
void vtkInterpolate(vtkTypedArray<T>* source_array, const vtkArraySlices& source_slices,
  const vtkArrayWeights& source_weights, const vtkArrayExtents& target_slice,
  vtkTypedArray<T>* target_array);

VTK_ABI_NAMESPACE_END
#include "vtkArrayInterpolate.txx"

#endif

// VTK-HeaderTest-Exclude: vtkArrayInterpolate.h
