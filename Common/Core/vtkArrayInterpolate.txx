// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkArrayInterpolate_txx
#define vtkArrayInterpolate_txx

#include "vtkArrayExtentsList.h"
#include "vtkArrayWeights.h"

VTK_ABI_NAMESPACE_BEGIN
template <typename T>
void vtkInterpolate(vtkTypedArray<T>* source_array, const vtkArrayExtentsList& source_slices,
  const vtkArrayWeights& source_weights, const vtkArrayExtents& target_slice,
  vtkTypedArray<T>* target_array)
{
  if (!target_array->GetExtents().Contains(target_slice))
  {
    vtkGenericWarningMacro(<< "Target array does not contain target slice.");
    return;
  }

  if (source_slices.GetCount() != source_weights.GetCount())
  {
    vtkGenericWarningMacro(<< "Source slice and weight counts must match.");
    return;
  }

  for (int i = 0; i != source_slices.GetCount(); ++i)
  {
    if (!target_slice.SameShape(source_slices[i]))
    {
      vtkGenericWarningMacro(<< "Source and target slice shapes must match: " << source_slices[i]
                             << " versus " << target_slice);
      return;
    }
  }

  // Zero-out the target storage ...
  const vtkIdType n_begin = 0;
  const vtkIdType n_end = target_slice.GetSize();
  vtkArrayCoordinates target_coordinates;
  for (vtkIdType n = n_begin; n != n_end; ++n)
  {
    target_slice.GetLeftToRightCoordinatesN(n, target_coordinates);
    target_array->SetValue(target_coordinates, 0);
  }

  // Accumulate results ...
  vtkArrayCoordinates source_coordinates;
  for (vtkIdType n = n_begin; n != n_end; ++n)
  {
    target_slice.GetLeftToRightCoordinatesN(n, target_coordinates);
    for (int source = 0; source != source_slices.GetCount(); ++source)
    {
      source_slices[source].GetLeftToRightCoordinatesN(n, source_coordinates);
      target_array->SetValue(target_coordinates,
        target_array->GetValue(target_coordinates) +
          (source_array->GetValue(source_coordinates) * source_weights[source]));
    }
  }
}

VTK_ABI_NAMESPACE_END
#endif
