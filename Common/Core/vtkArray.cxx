/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArray.cxx

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

#include "vtkArray.h"
#include "vtkDenseArray.h"
#include "vtkSparseArray.h"
#include "vtkVariant.h"

#include <algorithm>

//
// Standard functions
//


//----------------------------------------------------------------------------

vtkArray::vtkArray()
{
}

//----------------------------------------------------------------------------

vtkArray::~vtkArray()
{
}

//----------------------------------------------------------------------------

void vtkArray::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Name: " << this->Name << endl;

  os << indent << "Dimensions: " << this->GetDimensions() << endl;
  os << indent << "Extents: " << this->GetExtents() << endl;

  os << indent << "DimensionLabels:";
  for(DimensionT i = 0; i != this->GetDimensions(); ++i)
    os << " " << this->GetDimensionLabel(i);
  os << endl;

  os << indent << "Size: " << this->GetSize() << endl;
  os << indent << "NonNullSize: " << this->GetNonNullSize() << endl;
}

vtkArray* vtkArray::CreateArray(int StorageType, int ValueType)
{
  switch(StorageType)
  {
    case DENSE:
    {
      switch(ValueType)
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
        case VTK_UNICODE_STRING:
          return vtkDenseArray<vtkUnicodeString>::New();
        case VTK_VARIANT:
          return vtkDenseArray<vtkVariant>::New();
      }
      vtkGenericWarningMacro(<< "vtkArrary::CreateArray() cannot create array with unknown value type: " << vtkImageScalarTypeNameMacro(ValueType));
      return 0;
    }
    case SPARSE:
    {
      switch(ValueType)
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
        case VTK_UNICODE_STRING:
          return vtkSparseArray<vtkUnicodeString>::New();
        case VTK_VARIANT:
          return vtkSparseArray<vtkVariant>::New();
      }
      vtkGenericWarningMacro(<< "vtkArrary::CreateArray() cannot create array with unknown value type: " << vtkImageScalarTypeNameMacro(ValueType));
      return 0;
    }
  }

    vtkGenericWarningMacro(<< "vtkArrary::CreateArray() cannot create array with unknown storage type: " << StorageType);
    return 0;
}

void vtkArray::Resize(const CoordinateT i)
{
  this->Resize(vtkArrayExtents(vtkArrayRange(0, i)));
}

void vtkArray::Resize(const vtkArrayRange& i)
{
  this->Resize(vtkArrayExtents(i));
}

void vtkArray::Resize(const CoordinateT i, const CoordinateT j)
{
  this->Resize(vtkArrayExtents(vtkArrayRange(0, i), vtkArrayRange(0, j)));
}

void vtkArray::Resize(const vtkArrayRange& i, const vtkArrayRange& j)
{
  this->Resize(vtkArrayExtents(i, j));
}

void vtkArray::Resize(const CoordinateT i, const CoordinateT j, const CoordinateT k)
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

const vtkArrayRange vtkArray::GetExtent(DimensionT dimension)
{
  return this->GetExtents()[dimension];
}

vtkArray::DimensionT vtkArray::GetDimensions()
{
  return this->GetExtents().GetDimensions();
}

vtkTypeUInt64 vtkArray::GetSize()
{
  return this->GetExtents().GetSize();
}

void vtkArray::SetName(const vtkStdString& raw_name)
{
  // Don't allow newlines in array names ...
  vtkStdString name(raw_name);
  name.erase(std::remove(name.begin(), name.end(), '\r'), name.end());
  name.erase(std::remove(name.begin(), name.end(), '\n'), name.end());

  this->Name = name;
}

vtkStdString vtkArray::GetName()
{
  return this->Name;
}

void vtkArray::SetDimensionLabel(DimensionT i, const vtkStdString& raw_label)
{
  if(i < 0 || i >= this->GetDimensions())
  {
    vtkErrorMacro("Cannot set label for dimension " << i << " of a " << this->GetDimensions() << "-way array");
    return;
  }

  // Don't allow newlines in dimension labels ...
  vtkStdString label(raw_label);
  label.erase(std::remove(label.begin(), label.end(), '\r'), label.end());
  label.erase(std::remove(label.begin(), label.end(), '\n'), label.end());

  this->InternalSetDimensionLabel(i, label);
}

vtkStdString vtkArray::GetDimensionLabel(DimensionT i)
{
  if(i < 0 || i >= this->GetDimensions())
  {
    vtkErrorMacro("Cannot get label for dimension " << i << " of a " << this->GetDimensions() << "-way array");
    return "";
  }

  return this->InternalGetDimensionLabel(i);
}
