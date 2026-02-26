// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "ArrayConverters.hxx"

#include "vtkmDataArray.h"

#include "vtkmlib/DataSetUtils.h"
#include "vtkmlib/PortalTraits.h"

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSet.h>

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN
void ProcessFields(vtkDataSet* input, viskores::cont::DataSet& dataset, tovtkm::FieldsFlag fields)
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

      viskores::cont::Field pfield =
        tovtkm::Convert(array, vtkDataObject::FIELD_ASSOCIATION_POINTS);
      if (array->GetName() != pfield.GetName())
      {
        vtkGenericWarningMacro(<< "Could not convert point field `" << array->GetName()
                               << "` for Viskores filter.");
        continue;
      }
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

      viskores::cont::Field cfield = tovtkm::Convert(array, vtkDataObject::FIELD_ASSOCIATION_CELLS);
      if (array->GetName() != cfield.GetName())
      {
        vtkGenericWarningMacro(<< "Could not convert cell field `" << array->GetName()
                               << "` for Viskores filter.");
        continue;
      }
      dataset.AddField(cfield);
    }
  }
}

template <typename T>
viskores::cont::Field Convert(vtkmDataArray<T>* input, int association)
{
  // we need to switch on if we are a cell or point field first!
  // The problem is that the constructor signature for fields differ based
  // on if they are a cell or point field.
  if (association == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    return viskores::cont::make_FieldPoint(input->GetName(), input->GetVtkmUnknownArrayHandle());
  }
  else if (association == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    return viskores::cont::make_FieldCell(input->GetName(), input->GetVtkmUnknownArrayHandle());
  }

  return viskores::cont::Field();
}

template <typename VTK_TT>
static viskores::cont::Field TryConvert(vtkDataArray* input, int association)
{
  vtkAOSDataArrayTemplate<VTK_TT>* typedInAos =
    vtkAOSDataArrayTemplate<VTK_TT>::FastDownCast(input);
  if (typedInAos)
  {
    return Convert(typedInAos, association);
  }

  vtkSOADataArrayTemplate<VTK_TT>* typedInSoa =
    vtkSOADataArrayTemplate<VTK_TT>::FastDownCast(input);
  if (typedInSoa)
  {
    return Convert(typedInSoa, association);
  }

  vtkmDataArray<VTK_TT>* typedInVtkm = vtkmDataArray<VTK_TT>::SafeDownCast(input);
  if (typedInVtkm)
  {
    return Convert(typedInVtkm, association);
  }

  vtkConstantArray<VTK_TT>* typedInConst = vtkConstantArray<VTK_TT>::SafeDownCast(input);
  if (typedInConst)
  {
    return Convert(typedInConst, association);
  }

  vtkAffineArray<VTK_TT>* typedInAffine = vtkAffineArray<VTK_TT>::SafeDownCast(input);
  if (typedInAffine)
  {
    return Convert(typedInAffine, association);
  }

  return viskores::cont::Field{};
}

// determine the type and call the proper Convert routine
viskores::cont::Field Convert(vtkDataArray* input, int association)
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
  viskores::cont::Field field;
  switch (input->GetDataType())
  {
    vtkTemplateMacro(field = TryConvert<VTK_TT>(input, association););
    // end vtkTemplateMacro
  }
  return field;
}
VTK_ABI_NAMESPACE_END
} // namespace tovtkm

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN

bool ConvertArrays(const viskores::cont::DataSet& input, vtkDataSet* output)
{
  vtkPointData* pd = output->GetPointData();
  vtkCellData* cd = output->GetCellData();

  // Do not copy the coordinate systems, this is done in a higher level routine.
  for (auto i : GetFieldsIndicesWithoutCoords(input))
  {
    const viskores::cont::Field& f = input.GetField(i);
    if (f.GetData().GetNumberOfComponentsFlat() < 1)
    {
      vtkGenericWarningMacro("Viskores field "
        << f.GetName()
        << " does not have a fixed tuple size. This field will be unavailable in VTK.");
      continue;
    }
    vtkDataArray* vfield = Convert(f);
    if (vfield && f.GetAssociation() == viskores::cont::Field::Association::Points)
    {
      pd->AddArray(vfield);
      vfield->FastDelete();
    }
    else if (vfield && f.GetAssociation() == viskores::cont::Field::Association::Cells)
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
VTK_ABI_NAMESPACE_END
} // namespace fromvtkm
