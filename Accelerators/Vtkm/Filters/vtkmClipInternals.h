// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkmClipInternals_h
#define vtkmClipInternals_h

#include <vtkm/cont/DataSet.h>

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkImplicitFunction;

struct vtkmClip::internals
{
  static vtkm::cont::DataSet ExecuteClipWithImplicitFunction(
    vtkm::cont::DataSet& in, vtkImplicitFunction* clipFunction, bool insideOut);
  static vtkm::cont::DataSet ExecuteClipWithField(vtkm::cont::DataSet& in, vtkDataArray* scalars,
    int assoc, double value, bool insideOut, bool computeScalars);
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkmClipInternals.h
