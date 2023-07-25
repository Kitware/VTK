// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkmClip.h"
#include "vtkmClipInternals.h"
#include "vtkmlib/DataSetConverters.h"

#include <vtkm/filter/clean_grid/CleanGrid.h>
#include <vtkm/filter/contour/ClipWithField.h>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkm::cont::DataSet vtkmClip::internals::ExecuteClipWithField(vtkm::cont::DataSet& in,
  vtkDataArray* scalars, int assoc, double value, bool insideOut, bool computeScalars)
{
  vtkm::filter::contour::ClipWithField fieldFilter;
  if (!computeScalars)
  {
    // explicitly convert just the field we need
    auto inField = tovtkm::Convert(scalars, assoc);
    in.AddField(inField);
    // don't pass this field
    fieldFilter.SetFieldsToPass(
      vtkm::filter::FieldSelection(vtkm::filter::FieldSelection::Mode::None));
  }

  fieldFilter.SetActiveField(scalars->GetName(), vtkm::cont::Field::Association::Points);
  fieldFilter.SetClipValue(value);
  fieldFilter.SetInvertClip(insideOut);
  auto result = fieldFilter.Execute(in);

  // clean the output to remove unused points
  vtkm::filter::clean_grid::CleanGrid clean;
  result = clean.Execute(result);

  return result;
}
VTK_ABI_NAMESPACE_END
