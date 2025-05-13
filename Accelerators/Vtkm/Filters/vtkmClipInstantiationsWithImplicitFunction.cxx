// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkmClip.h"
#include "vtkmClipInternals.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/ImplicitFunctionConverter.h"

#include <viskores/filter/clean_grid/CleanGrid.h>
#include <viskores/filter/contour/ClipWithImplicitFunction.h>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
viskores::cont::DataSet vtkmClip::internals::ExecuteClipWithImplicitFunction(
  viskores::cont::DataSet& in, vtkImplicitFunction* clipFunction, bool insideOut)
{
  tovtkm::ImplicitFunctionConverter clipFunctionConverter;
  clipFunctionConverter.Set(clipFunction);
  auto function = clipFunctionConverter.Get();

  viskores::cont::DataSet result;
  viskores::filter::contour::ClipWithImplicitFunction functionFilter;
  functionFilter.SetImplicitFunction(function);
  functionFilter.SetInvertClip(insideOut);
  result = functionFilter.Execute(in);

  return result;
}
VTK_ABI_NAMESPACE_END
