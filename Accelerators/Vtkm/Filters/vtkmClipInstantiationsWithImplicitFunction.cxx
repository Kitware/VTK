/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkmClipInstantiationsWithImplicitFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

  // clean the output to remove unused points
  vtkm::filter::clean_grid::CleanGrid clean;
  result = clean.Execute(result);

  return result;
}
VTK_ABI_NAMESPACE_END
