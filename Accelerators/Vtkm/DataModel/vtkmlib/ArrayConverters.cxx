//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "ArrayConverters.hxx"

#include "vtkmDataArray.h"
#include "vtkmFilterPolicy.h"

#include "vtkmlib/PortalTraits.h"

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

namespace tovtkm
{
void ProcessFields(vtkDataSet* input, vtkm::cont::DataSet& dataset, tovtkm::FieldsFlag fields)
{
  if ((fields & tovtkm::FieldsFlag::Points) != tovtkm::FieldsFlag::None)
  {
    vtkPointData* pd = input->GetPointData();
    for (int i = 0; i < pd->GetNumberOfArrays(); i++)
    {
      vtkDataArray* array = pd->GetArray(i);
      if (array == nullptr)
      {
        continue;
      }

      vtkm::cont::Field pfield = tovtkm::Convert(array, vtkDataObject::FIELD_ASSOCIATION_POINTS);
      dataset.AddField(pfield);
    }
  }

  if ((fields & tovtkm::FieldsFlag::Cells) != tovtkm::FieldsFlag::None)
  {
    vtkCellData* cd = input->GetCellData();
    for (int i = 0; i < cd->GetNumberOfArrays(); i++)
    {
      vtkDataArray* array = cd->GetArray(i);
      if (array == nullptr)
      {
        continue;
      }

      vtkm::cont::Field cfield = tovtkm::Convert(array, vtkDataObject::FIELD_ASSOCIATION_CELLS);
      dataset.AddField(cfield);
    }
  }
}

template <typename T>
vtkm::cont::Field Convert(vtkmDataArray<T>* input, int association)
{
  // we need to switch on if we are a cell or point field first!
  // The problem is that the constructor signature for fields differ based
  // on if they are a cell or point field.
  if (association == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    return vtkm::cont::make_FieldPoint(input->GetName(), input->GetVtkmUnknownArrayHandle());
  }
  else if (association == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    return vtkm::cont::make_FieldCell(input->GetName(), input->GetVtkmUnknownArrayHandle());
  }

  return vtkm::cont::Field();
}

// determine the type and call the proper Convert routine
vtkm::cont::Field Convert(vtkDataArray* input, int association)
{
  // The association will tell us if we have a cell or point field

  // We need to properly deduce the ValueType of the array. This means
  // that we need to switch on Float/Double/Int, and then figure out the
  // number of components. The upside is that the Convert Method can internally
  // figure out the number of components, and not have to generate a lot
  // of template to do so

  // Investigate using vtkArrayDispatch, AOS for all types, and than SOA for
  // just
  // float/double
  vtkm::cont::Field field;
  switch (input->GetDataType())
  {
    vtkTemplateMacro(
      vtkAOSDataArrayTemplate<VTK_TT>* typedIn1 =
        vtkAOSDataArrayTemplate<VTK_TT>::FastDownCast(input);
      if (typedIn1) { field = Convert(typedIn1, association); } else {
        vtkSOADataArrayTemplate<VTK_TT>* typedIn2 =
          vtkSOADataArrayTemplate<VTK_TT>::FastDownCast(input);
        if (typedIn2)
        {
          field = Convert(typedIn2, association);
        }
        else
        {
          vtkmDataArray<VTK_TT>* typedIn3 = vtkmDataArray<VTK_TT>::SafeDownCast(input);
          if (typedIn3)
          {
            field = Convert(typedIn3, association);
          }
        }
      });
    // end vtkTemplateMacro
  }
  return field;
}
} // namespace tovtkm

namespace fromvtkm
{

bool ConvertArrays(const vtkm::cont::DataSet& input, vtkDataSet* output)
{
  vtkPointData* pd = output->GetPointData();
  vtkCellData* cd = output->GetCellData();

  vtkm::IdComponent numFields = input.GetNumberOfFields();
  for (vtkm::IdComponent i = 0; i < numFields; ++i)
  {
    const vtkm::cont::Field& f = input.GetField(i);
    vtkDataArray* vfield = Convert(f);
    if (vfield && f.GetAssociation() == vtkm::cont::Field::Association::Points)
    {
      pd->AddArray(vfield);
      vfield->FastDelete();
    }
    else if (vfield && f.GetAssociation() == vtkm::cont::Field::Association::Cells)
    {
      cd->AddArray(vfield);
      vfield->FastDelete();
    }
    else if (vfield)
    {
      vfield->Delete();
    }
  }
  return true;
}
} // namespace fromvtkm
