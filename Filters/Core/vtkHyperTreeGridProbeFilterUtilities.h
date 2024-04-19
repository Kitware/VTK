// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @namespace vtkHyperTreeGridProbeFilterUtilities
 * @brief internal utilities for vtk(P)HyperTreeGridProbeFilter
 *
 * Utility methods used by both vtkHyperTreeGridProbeFilter and
 * vtkPHyperTreeGridProbeFilter.
 */

#ifndef vtkHyperTreeGridProbeFilterUtilities_h
#define vtkHyperTreeGridProbeFilterUtilities_h

#include "vtkFiltersCoreModule.h" // For export Macro
#include "vtkObject.h"            // For VTK_ABI_NAMESPACE

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
VTK_ABI_NAMESPACE_END

namespace vtkHyperTreeGridProbeFilterUtilities
{
VTK_ABI_NAMESPACE_BEGIN

constexpr vtkIdType HANDLES_INVALID_ID = -1;

/**
 * Fill array with default value depending on the value type:
 * - NaN values for double and float array
 * - "" for string array
 * - 0 for integral values
 */
VTKFILTERSCORE_EXPORT void FillDefaultArray(vtkAbstractArray* aa);

VTK_ABI_NAMESPACE_END
}

#endif
// VTK-HeaderTest-Exclude: vtkHyperTreeGridProbeFilterUtilities.h
