// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkVariantCast.h"
#include "vtkVariantCreate.h"

VTK_ABI_NAMESPACE_BEGIN
template <typename T>
void vtkTypedArray<T>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkTypedArray<T>::Superclass::PrintSelf(os, indent);
}

template <typename T>
vtkVariant vtkTypedArray<T>::GetVariantValue(const vtkArrayCoordinates& coordinates)
{
  return vtkVariantCreate<T>(this->GetValue(coordinates));
}

template <typename T>
vtkVariant vtkTypedArray<T>::GetVariantValueN(SizeT n)
{
  return vtkVariantCreate<T>(this->GetValueN(n));
}

template <typename T>
void vtkTypedArray<T>::SetVariantValue(
  const vtkArrayCoordinates& coordinates, const vtkVariant& value)
{
  this->SetValue(coordinates, vtkVariantCast<T>(value));
}

template <typename T>
void vtkTypedArray<T>::SetVariantValueN(SizeT n, const vtkVariant& value)
{
  this->SetValueN(n, vtkVariantCast<T>(value));
}

template <typename T>
void vtkTypedArray<T>::CopyValue(vtkArray* source, const vtkArrayCoordinates& source_coordinates,
  const vtkArrayCoordinates& target_coordinates)
{
  if (!source->IsA(this->GetClassName()))
  {
    vtkWarningMacro("source and target array data types do not match");
    return;
  }

  this->SetValue(
    target_coordinates, static_cast<vtkTypedArray<T>*>(source)->GetValue(source_coordinates));
}

template <typename T>
void vtkTypedArray<T>::CopyValue(
  vtkArray* source, SizeT source_index, const vtkArrayCoordinates& target_coordinates)
{
  if (!source->IsA(this->GetClassName()))
  {
    vtkWarningMacro("source and target array data types do not match");
    return;
  }

  this->SetValue(
    target_coordinates, static_cast<vtkTypedArray<T>*>(source)->GetValueN(source_index));
}

template <typename T>
void vtkTypedArray<T>::CopyValue(
  vtkArray* source, const vtkArrayCoordinates& source_coordinates, SizeT target_index)
{
  if (!source->IsA(this->GetClassName()))
  {
    vtkWarningMacro("source and target array data types do not match");
    return;
  }

  this->SetValueN(
    target_index, static_cast<vtkTypedArray<T>*>(source)->GetValue(source_coordinates));
}
VTK_ABI_NAMESPACE_END
