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

vtkCxxRevisionMacro(vtkUnicodeStringArray, "1.2");
vtkStandardNewMacro(vtkUnicodeStringArray);

vtkUnicodeStringArray::vtkUnicodeStringArray(vtkIdType)
{
}

vtkUnicodeStringArray::~vtkUnicodeStringArray()
{
}

void vtkUnicodeStringArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkUnicodeStringArray::Allocate(vtkIdType sz, vtkIdType)
{
  this->Storage.reserve(sz);
  this->DataChanged();
  return 1;
}

void vtkUnicodeStringArray::Initialize()
{
  this->Storage.clear();
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
  this->Storage.resize(number);
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

  this->Storage[i] = array->Storage[j];
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

  this->Storage.insert(this->Storage.begin() + i, array->Storage[j]);
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

  this->Storage.push_back(array->Storage[j]);
  this->DataChanged();
  return this->Storage.size() - 1;
}

void* vtkUnicodeStringArray::GetVoidPointer(vtkIdType id)
{
  return &this->Storage[id];
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

  this->Storage = array->Storage;
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
  StorageT(this->Storage).swap(this->Storage);
  this->DataChanged();
}

int vtkUnicodeStringArray::Resize(vtkIdType numTuples)
{
  this->Storage.resize(numTuples);
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
  for(StorageT::size_type i = 0; i != this->Storage.size(); ++i)
    {
    count += this->Storage[i].byte_count();
    count += sizeof(vtkUnicodeString);
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
  this->MaxId = this->Storage.size() - 1;
}

void vtkUnicodeStringArray::ClearLookup()
{
}

vtkIdType vtkUnicodeStringArray::InsertNextValue(const vtkUnicodeString& value)
{
  this->Storage.push_back(value);
  this->DataChanged();
  return this->Storage.size() - 1;
}

void vtkUnicodeStringArray::SetValue(vtkIdType i, const vtkUnicodeString& value)
{
  this->Storage[i] = value;
  this->DataChanged();
}

vtkUnicodeString& vtkUnicodeStringArray::GetValue(vtkIdType i)
{
  return this->Storage[i];
}

void vtkUnicodeStringArray::InsertNextUTF8Value(const char* value)
{
  this->InsertNextValue(vtkUnicodeString::from_utf8(value));
}

const char* vtkUnicodeStringArray::GetUTF8Value(vtkIdType i)
{
  return this->Storage[i].utf8_str();
}

