// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkVectorOperators_h
#define vtkVectorOperators_h

// This file used to set operators that enhance the vtkVector classes.
// We reimplemented them directly in vtkVector.h, and this file is now only kept
// for deprecation purpose and should be removed once we hit the deprecation milestone.
#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_4_0
#include "vtkVector.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{
VTK_DEPRECATED_IN_9_4_0("vtkVectorOperators.h is deprecated use vtkVector.h instead")
constexpr int vtkVectorOperators_h_is_deprecated = 0;
constexpr int please_dont_use_vtkVectorOperators_h = vtkVectorOperators_h_is_deprecated;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkVectorOperators.h
