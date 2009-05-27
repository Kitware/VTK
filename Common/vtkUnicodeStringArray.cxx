/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnicodeStringArray.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkUnicodeStringArray.h"

#include <vtkstd/vector>

class vtkUnicodeStringArray::Internals
{
public:
  typedef vtkstd::vector<vtkUnicodeString> StorageT;
  StorageT Storage;
};

vtkCxxRevisionMacro(vtkUnicodeStringArray, "1.8");
vtkStandardNewMacro(vtkUnicodeStringArray);

vtkUnicodeStringArray::vtkUnicodeStringArray(vtkIdType)
{
  this->Implementation = new Internals;
}

vtkUnicodeStringArray::~vtkUnicodeStringArray()
{
  delete this->Implementation;
}

void vtkUnicodeStringArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkUnicodeStringArray::Allocate(vtkIdType sz, vtkIdType)
{
  this->Implementation->Storage.reserve(sz);
  this->DataChanged();
  return 1;
}

void vtkUnicodeStringArray::Initialize()
{
  this->Implementation->Storage.clear();
  this->DataChanged();
}

int vtkUnicodeStringArray::GetDataType()
{
  return VTK_UNICODE_STRING;
}

int vtkUnicodeStringArray::GetDataTypeSize()
{
  return 0;
}

int vtkUnicodeStringArray::GetElementComponentSize()
{
  return sizeof(vtkUnicodeString::value_type);
}

void vtkUnicodeStringArray::SetNumberOfTuples(vtkIdType number)
{
  this->Implementation->Storage.resize(number);
  this->DataChanged();
}

void vtkUnicodeStringArray::SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source)
{
  vtkUnicodeStringArray* const array = vtkUnicodeStringArray::SafeDownCast(source);
  if(!array)
    {
    vtkWarningMacro("Input and output array data types do not match.");
    return;
    }

  this->Implementation->Storage[i] = array->Implementation->Storage[j];
  this->DataChanged();
}

void vtkUnicodeStringArray::InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source)
{
  vtkUnicodeStringArray* const array = vtkUnicodeStringArray::SafeDownCast(source);
  if(!array)
    {
    vtkWarningMacro("Input and output array data types do not match.");
    return;
    }

  if(static_cast<vtkIdType>(this->Implementation->Storage.size()) <= i)
    this->Implementation->Storage.resize(i + 1);

  this->Implementation->Storage[i] = array->Implementation->Storage[j];
  this->DataChanged();
}

vtkIdType vtkUnicodeStringArray::InsertNextTuple(vtkIdType j, vtkAbstractArray* source)
{
  vtkUnicodeStringArray* const array = vtkUnicodeStringArray::SafeDownCast(source);
  if(!array)
    {
    vtkWarningMacro("Input and output array data types do not match.");
    return 0;
    }

  this->Implementation->Storage.push_back(array->Implementation->Storage[j]);
  this->DataChanged();
  return this->Implementation->Storage.size() - 1;
}

void* vtkUnicodeStringArray::GetVoidPointer(vtkIdType id)
{
  return &this->Implementation->Storage[id];
}

void vtkUnicodeStringArray::DeepCopy(vtkAbstractArray* da)
{
  if(!da)
    return;

  if(this == da)
    return;

  vtkUnicodeStringArray* const array = vtkUnicodeStringArray::SafeDownCast(da);
  if(!array)
    {
    vtkWarningMacro("Input and output array data types do not match.");
    return;
    }

  this->Implementation->Storage = array->Implementation->Storage;
  this->DataChanged();
}

void vtkUnicodeStringArray::InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
    vtkAbstractArray* source,  double* weights)
{
  if(this->GetDataType() != source->GetDataType())
    {
    vtkErrorMacro("Cannot CopyValue from array of type " 
      << source->GetDataTypeAsString());
    return;
    }

  if (ptIndices->GetNumberOfIds() == 0)
    {
    // nothing to do.
    return;
    }

  // We use nearest neighbour for interpolating strings.
  // First determine which is the nearest neighbour using the weights-
  // it's the index with maximum weight.
  vtkIdType nearest = ptIndices->GetId(0);
  double max_weight = weights[0];
  for (int k=1; k < ptIndices->GetNumberOfIds(); k++)
    {
    if (weights[k] > max_weight)
      {
      nearest = ptIndices->GetId(k);
      max_weight = weights[k];
      }
    }

  this->InsertTuple(i, nearest, source);
}

void vtkUnicodeStringArray::InterpolateTuple(vtkIdType i, 
    vtkIdType id1, vtkAbstractArray* source1, 
    vtkIdType id2, vtkAbstractArray* source2, double t)
{
  if (source1->GetDataType() != this->GetDataType() || 
    source2->GetDataType() != this->GetDataType())
    {
    vtkErrorMacro("All arrays to InterpolateValue() must be of same type.");
    return;
    }

  if (t >= 0.5)
    {
    // Use p2
    this->InsertTuple(i, id2, source2);
    }
  else
    {
    // Use p1.
    this->InsertTuple(i, id1, source1); 
    }
}

void vtkUnicodeStringArray::Squeeze()
{
  Internals::StorageT(this->Implementation->Storage).swap(this->Implementation->Storage);
  this->DataChanged();
}

int vtkUnicodeStringArray::Resize(vtkIdType numTuples)
{
  this->Implementation->Storage.resize(numTuples);
  this->DataChanged();
  return 1;
}

void vtkUnicodeStringArray::SetVoidArray(void*, vtkIdType, int)
{
  vtkErrorMacro("Not implemented.");
}

unsigned long vtkUnicodeStringArray::GetActualMemorySize()
{
  unsigned long count = 0;
  for(Internals::StorageT::size_type i = 0; i != this->Implementation->Storage.size(); ++i)
    {
    count += static_cast<unsigned long>(this->Implementation->Storage[i].byte_count());
    count += static_cast<unsigned long>(sizeof(vtkUnicodeString));
    }
  return count;
}

int vtkUnicodeStringArray::IsNumeric()
{
  return 0;
}

vtkArrayIterator* vtkUnicodeStringArray::NewIterator()
{
  vtkErrorMacro("Not implemented.");
  return 0;
}

vtkVariant vtkUnicodeStringArray::GetVariantValue(vtkIdType idx)
{
  return this->GetValue(idx);
}

vtkIdType vtkUnicodeStringArray::LookupValue(vtkVariant)
{
  vtkErrorMacro("Not implemented.");
  return -1;
}

void vtkUnicodeStringArray::LookupValue(vtkVariant, vtkIdList* ids)
{
  vtkErrorMacro("Not implemented.");
  ids->Reset();
}

void vtkUnicodeStringArray::InsertVariantValue(vtkIdType, vtkVariant)
{
  vtkErrorMacro("Not implemented.");
}

void vtkUnicodeStringArray::DataChanged()
{
  this->MaxId = this->Implementation->Storage.size() - 1;
}

void vtkUnicodeStringArray::ClearLookup()
{
}

vtkIdType vtkUnicodeStringArray::InsertNextValue(const vtkUnicodeString& value)
{
  this->Implementation->Storage.push_back(value);
  this->DataChanged();
  return this->Implementation->Storage.size() - 1;
}

void vtkUnicodeStringArray::SetValue(vtkIdType i, const vtkUnicodeString& value)
{
  this->Implementation->Storage[i] = value;
  this->DataChanged();
}

vtkUnicodeString& vtkUnicodeStringArray::GetValue(vtkIdType i)
{
  return this->Implementation->Storage[i];
}

void vtkUnicodeStringArray::InsertNextUTF8Value(const char* value)
{
  this->InsertNextValue(vtkUnicodeString::from_utf8(value));
}

void vtkUnicodeStringArray::SetUTF8Value(vtkIdType i, const char* value)
{
  this->SetValue(i, vtkUnicodeString::from_utf8(value));
}

const char* vtkUnicodeStringArray::GetUTF8Value(vtkIdType i)
{
  return this->Implementation->Storage[i].utf8_str();
}

