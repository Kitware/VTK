// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkmClip.h"
#include "vtkmClipInternals.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/ImplicitFunctionConverter.h"

#include <vtkm/filter/clean_grid/CleanGrid.h>
#include <vtkm/filter/contour/ClipWithImplicitFunction.h>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkm::cont::DataSet vtkmClip::internals::ExecuteClipWithImplicitFunction(
  vtkm::cont::DataSet& in, vtkImplicitFunction* clipFunction, bool insideOut)
{
  tovtkm::ImplicitFunctionConverter clipFunctionConverter;
  clipFunctionConverter.Set(clipFunction);
  auto function = clipFunctionConverter.Get();

  vtkm::cont::DataSet result;
  vtkm::filter::contour::ClipWithImplicitFunction functionFilter;
  functionFilter.SetImplicitFunction(function);
  functionFilter.SetInvertClip(insideOut);
  result = functionFilter.Execute(in);

  return result;
}
VTK_ABI_NAMESPACE_END
