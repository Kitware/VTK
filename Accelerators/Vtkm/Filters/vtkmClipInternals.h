// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkmClipInternals_h
#define vtkmClipInternals_h

#include <viskores/cont/DataSet.h>

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkImplicitFunction;

struct vtkmClip::internals
{
  static viskores::cont::DataSet ExecuteClipWithImplicitFunction(
    viskores::cont::DataSet& in, vtkImplicitFunction* clipFunction, bool insideOut);
  static viskores::cont::DataSet ExecuteClipWithField(viskores::cont::DataSet& in,
    vtkDataArray* scalars, int assoc, double value, bool insideOut, bool computeScalars);
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkmClipInternals.h
