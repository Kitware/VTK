/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkmClipInstantiationsWithField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkmClipInternals.h"
#include "vtkmlib/DataSetConverters.h"

#include <vtkm/filter/ClipWithField.h>

//------------------------------------------------------------------------------
vtkm::cont::DataSet vtkmClip::internals::ExecuteClipWithField(
  vtkm::cont::DataSet& in, vtkDataArray* scalars, int assoc)
{
  vtkm::filter::ClipWithField fieldFilter;
  if (!this->ComputeScalars)
  {
    // explicitly convert just the field we need
    auto inField = tovtkm::Convert(scalars, assoc);
    in.AddField(inField);
    // don't pass this field
    fieldFilter.SetFieldsToPass(
      vtkm::filter::FieldSelection(vtkm::filter::FieldSelection::MODE_NONE));
  }

  fieldFilter.SetActiveField(scalars->GetName(), vtkm::cont::Field::Association::POINTS);
  fieldFilter.SetClipValue(this->ClipValue);
  return fieldFilter.Execute(in);
}
