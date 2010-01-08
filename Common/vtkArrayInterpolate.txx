/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayInterpolate.txx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkArrayInterpolate_txx
#define __vtkArrayInterpolate_txx

#include "vtkArraySlices.h"
#include "vtkArrayWeights.h"

template<typename T>
void vtkInterpolate(
  vtkTypedArray<T>* source_array,
  const vtkArraySlices& source_slices,
  const vtkArrayWeights& source_weights,
  const vtkArraySlice& target_slice,
  vtkTypedArray<T>* target_array)
{
  const vtkArrayExtents target_extents = target_slice.GetExtents();
  
  if(target_extents.GetDimensions() != target_array->GetDimensions())
    {
    vtkGenericWarningMacro(<< "target slice must match target array dimensions");
    return;
    }
  
  if(source_slices.GetCount() != source_weights.GetCount())
    {
    vtkGenericWarningMacro(<< "source slice and weight counts must match");
    return;
    }

  for(int i = 0; i != source_slices.GetCount(); ++i)
    {
    if(source_slices[i].GetExtents() != target_extents)
      {
      vtkGenericWarningMacro(<< "source and target slice extents must match");
      return;
      }
    }
    
  // Zero-out the target storage ...
  const vtkIdType n_begin = 0;
  const vtkIdType n_end = target_extents.GetSize();
  vtkArrayCoordinates target_coordinates;
  for(vtkIdType n = n_begin; n != n_end; ++n)
    {
    target_slice.GetCoordinatesN(n, target_coordinates);
    target_array->SetValue(target_coordinates, 0);
    }

  // Accumulate results ...
  vtkArrayCoordinates source_coordinates;
  for(vtkIdType n = n_begin; n != n_end; ++n)
    {
    target_slice.GetCoordinatesN(n, target_coordinates);
    for(int source = 0; source != source_slices.GetCount(); ++source)
      {
      source_slices[source].GetCoordinatesN(n, source_coordinates);
      target_array->SetValue(target_coordinates, target_array->GetValue(target_coordinates) + (source_array->GetValue(source_coordinates) * source_weights[source]));
      }
    }
}

#endif

