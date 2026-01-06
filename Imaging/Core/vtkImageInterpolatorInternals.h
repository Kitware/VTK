// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInterpolatorInternals
 * @brief   internals for vtkImageInterpolator
 */

#ifndef vtkImageInterpolatorInternals_h
#define vtkImageInterpolatorInternals_h

// This file is obsolete and will be removed in a future version of VTK.
// C++ can't [[deprecate]] a file, so we can't use VTK_DEPRECATED_IN_9_6_0
#if VTK_DEPRECATION_LEVEL >= VTK_VERSION_CHECK(9, 5, 20250513)
#if defined(__GNUC__) || __cplusplus >= 202302L
#warning "Please include vtkInterpolationMath.h instead."
#else // this is a rarely used internal header, it wasn't even exported
#error "Please include vtkInterpolationMath.h instead."
#endif
#endif

// The contents of this header were moved to the following public header:
#include "vtkInterpolationMath.h"

#endif
// VTK-HeaderTest-Exclude: vtkImageInterpolatorInternals.h
