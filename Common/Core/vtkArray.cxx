// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkArray.h"
#include "vtkDenseArray.h"
#include "vtkSparseArray.h"
#include "vtkVariant.h"

#include <algorithm>

//
// Standard functions
//

//------------------------------------------------------------------------------

VTK_ABI_NAMESPACE_BEGIN
vtkArray::vtkArray() = default;

//------------------------------------------------------------------------------

vtkArray::~vtkArray() = default;

//------------------------------------------------------------------------------

void vtkArray::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Name: " << this->Name << endl;

  os << indent << "Dimensions: " << this->GetDimensions() << endl;
  os << indent << "Extents: " << this->GetExtents() << endl;

  os << indent << "DimensionLabels:";
  for (DimensionT i = 0; i != this->GetDimensions(); ++i)
    os << " " << this->GetDimensionLabel(i);
  os << endl;

  os << indent << "Size: " << this->GetSize() << endl;
  os << indent << "NonNullSize: " << this->GetNonNullSize() << endl;
}

vtkArray* vtkArray::CreateArray(int StorageType, int ValueType)
{
  switch (StorageType)
  {
    case DENSE:
    {
      switch (ValueType)
      {
        case VTK_CHAR:
          return vtkDenseArray<char>::New();
        case VTK_SIGNED_CHAR:
          return vtkDenseArray<signed char>::New();
        case VTK_UNSIGNED_CHAR:
          return vtkDenseArray<unsigned char>::New();
        case VTK_SHORT:
          return vtkDenseArray<short>::New();
        case VTK_UNSIGNED_SHORT:
          return vtkDenseArray<unsigned short>::New();
        case VTK_INT:
          return vtkDenseArray<int>::New();
        case VTK_UNSIGNED_INT:
          return vtkDenseArray<unsigned int>::New();
        case VTK_LONG:
          return vtkDenseArray<long>::New();
        case VTK_UNSIGNED_LONG:
          return vtkDenseArray<unsigned long>::New();
        case VTK_LONG_LONG:
          return vtkDenseArray<long long>::New();
        case VTK_UNSIGNED_LONG_LONG:
          return vtkDenseArray<unsigned long long>::New();
        case VTK_FLOAT:
          return vtkDenseArray<float>::New();
        case VTK_DOUBLE:
          return vtkDenseArray<double>::New();
        case VTK_ID_TYPE:
          return vtkDenseArray<vtkIdType>::New();
        case VTK_STRING:
          return vtkDenseArray<vtkStdString>::New();
        case VTK_VARIANT:
          return vtkDenseArray<vtkVariant>::New();
      }
      vtkGenericWarningMacro(
        << "vtkArrary::CreateArray() cannot create array with unknown value type: "
        << vtkImageScalarTypeNameMacro(ValueType));
      return nullptr;
    }
    case SPARSE:
    {
      switch (ValueType)
      {
        case VTK_CHAR:
          return vtkSparseArray<char>::New();
        case VTK_SIGNED_CHAR:
          return vtkSparseArray<signed char>::New();
        case VTK_UNSIGNED_CHAR:
          return vtkSparseArray<unsigned char>::New();
        case VTK_SHORT:
          return vtkSparseArray<short>::New();
        case VTK_UNSIGNED_SHORT:
          return vtkSparseArray<unsigned short>::New();
        case VTK_INT:
          return vtkSparseArray<int>::New();
        case VTK_UNSIGNED_INT:
          return vtkSparseArray<unsigned int>::New();
        case VTK_LONG:
          return vtkSparseArray<long>::New();
        case VTK_UNSIGNED_LONG:
          return vtkSparseArray<unsigned long>::New();
        case VTK_LONG_LONG:
          return vtkSparseArray<long long>::New();
        case VTK_UNSIGNED_LONG_LONG:
          return vtkSparseArray<unsigned long long>::New();
        case VTK_FLOAT:
          return vtkSparseArray<float>::New();
        case VTK_DOUBLE:
          return vtkSparseArray<double>::New();
        case VTK_ID_TYPE:
          return vtkSparseArray<vtkIdType>::New();
        case VTK_STRING:
          return vtkSparseArray<vtkStdString>::New();
        case VTK_VARIANT:
          return vtkSparseArray<vtkVariant>::New();
      }
      vtkGenericWarningMacro(
        << "vtkArrary::CreateArray() cannot create array with unknown value type: "
        << vtkImageScalarTypeNameMacro(ValueType));
      return nullptr;
    }
  }

  vtkGenericWarningMacro(
    << "vtkArrary::CreateArray() cannot create array with unknown storage type: " << StorageType);
  return nullptr;
}

void vtkArray::Resize(CoordinateT i)
{
  this->Resize(vtkArrayExtents(vtkArrayRange(0, i)));
}

void vtkArray::Resize(const vtkArrayRange& i)
{
  this->Resize(vtkArrayExtents(i));
}

void vtkArray::Resize(CoordinateT i, CoordinateT j)
{
  this->Resize(vtkArrayExtents(vtkArrayRange(0, i), vtkArrayRange(0, j)));
}

void vtkArray::Resize(const vtkArrayRange& i, const vtkArrayRange& j)
{
  this->Resize(vtkArrayExtents(i, j));
}

void vtkArray::Resize(CoordinateT i, CoordinateT j, CoordinateT k)
{
  this->Resize(vtkArrayExtents(vtkArrayRange(0, i), vtkArrayRange(0, j), vtkArrayRange(0, k)));
}

void vtkArray::Resize(const vtkArrayRange& i, const vtkArrayRange& j, const vtkArrayRange& k)
{
  this->Resize(vtkArrayExtents(i, j, k));
}

void vtkArray::Resize(const vtkArrayExtents& extents)
{
  this->InternalResize(extents);
}

vtkArrayRange vtkArray::GetExtent(DimensionT dimension)
{
  return this->GetExtents()[dimension];
}

vtkArray::DimensionT vtkArray::GetDimensions() VTK_FUTURE_CONST
{
  return this->GetExtents().GetDimensions();
}

vtkTypeUInt64 vtkArray::GetSize() VTK_FUTURE_CONST
{
  return this->GetExtents().GetSize();
}

void vtkArray::SetName(const vtkStdString& raw_name)
{
  // Don't allow newlines in array names ...
  std::string name(raw_name);
  name.erase(std::remove(name.begin(), name.end(), '\r'), name.end());
  name.erase(std::remove(name.begin(), name.end(), '\n'), name.end());

  this->Name = name;
}

vtkStdString vtkArray::GetName() VTK_FUTURE_CONST
{
  return this->Name;
}

void vtkArray::SetDimensionLabel(DimensionT i, const vtkStdString& raw_label)
{
  if (i < 0 || i >= this->GetDimensions())
  {
    vtkErrorMacro(
      "Cannot set label for dimension " << i << " of a " << this->GetDimensions() << "-way array");
    return;
  }

  // Don't allow newlines in dimension labels ...
  std::string label(raw_label);
  label.erase(std::remove(label.begin(), label.end(), '\r'), label.end());
  label.erase(std::remove(label.begin(), label.end(), '\n'), label.end());

  this->InternalSetDimensionLabel(i, label);
}

vtkStdString vtkArray::GetDimensionLabel(DimensionT i) VTK_FUTURE_CONST
{
  if (i < 0 || i >= this->GetDimensions())
  {
    vtkErrorMacro(
      "Cannot get label for dimension " << i << " of a " << this->GetDimensions() << "-way array");
    return "";
  }

  return this->InternalGetDimensionLabel(i);
}
VTK_ABI_NAMESPACE_END
