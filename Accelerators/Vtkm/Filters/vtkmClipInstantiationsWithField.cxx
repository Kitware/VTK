// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkmClip.h"
#include "vtkmClipInternals.h"
#include "vtkmlib/DataSetConverters.h"

#include <viskores/filter/clean_grid/CleanGrid.h>
#include <viskores/filter/contour/ClipWithField.h>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
viskores::cont::DataSet vtkmClip::internals::ExecuteClipWithField(viskores::cont::DataSet& in,
  vtkDataArray* scalars, int assoc, double value, bool insideOut, bool computeScalars)
{
  viskores::filter::contour::ClipWithField fieldFilter;
  if (!computeScalars)
  {
    // explicitly convert just the field we need
    auto inField = tovtkm::Convert(scalars, assoc);
    in.AddField(inField);
    // don't pass this field
    fieldFilter.SetFieldsToPass(
      viskores::filter::FieldSelection(viskores::filter::FieldSelection::Mode::None));
  }

  fieldFilter.SetActiveField(scalars->GetName(), viskores::cont::Field::Association::Points);
  fieldFilter.SetClipValue(value);
  fieldFilter.SetInvertClip(insideOut);
  auto result = fieldFilter.Execute(in);

  return result;
}
VTK_ABI_NAMESPACE_END
