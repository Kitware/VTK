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

#include <vector>
#include <algorithm>

class vtkUnicodeStringArray::Implementation
{
public:
  typedef std::vector<vtkUnicodeString> StorageT;
  StorageT Storage;
};

vtkStandardNewMacro(vtkUnicodeStringArray);

vtkUnicodeStringArray::vtkUnicodeStringArray()
{
  this->Internal = new Implementation;
}

vtkUnicodeStringArray::~vtkUnicodeStringArray()
{
  delete this->Internal;
}

void vtkUnicodeStringArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkUnicodeStringArray::Allocate(vtkIdType sz, vtkIdType)
{
  this->Internal->Storage.reserve(sz);
  this->DataChanged();
  return 1;
}

void vtkUnicodeStringArray::Initialize()
{
  this->Internal->Storage.clear();
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
  this->Internal->Storage.resize(number);
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

  this->Internal->Storage[i] = array->Internal->Storage[j];
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

  if(static_cast<vtkIdType>(this->Internal->Storage.size()) <= i)
    this->Internal->Storage.resize(i + 1);

  this->Internal->Storage[i] = array->Internal->Storage[j];
  this->DataChanged();
}

void vtkUnicodeStringArray::InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                                         vtkAbstractArray *source)
{
  vtkUnicodeStringArray* const array =
      vtkUnicodeStringArray::SafeDownCast(source);
  if(!array)
    {
    vtkWarningMacro("Input and output array data types do not match.");
    return;
    }

  vtkIdType numIds = dstIds->GetNumberOfIds();
  if (srcIds->GetNumberOfIds() != numIds)
    {
    vtkWarningMacro("Input and output id array sizes do not match.");
    return;
    }

  // Find maximum destination id and resize if needed
  vtkIdType maxDstId = 0;
  for (vtkIdType idIndex = 0; idIndex < numIds; ++idIndex)
    {
    maxDstId = std::max(maxDstId, dstIds->GetId(idIndex));
    }

  if (static_cast<vtkIdType>(this->Internal->Storage.size()) <= maxDstId)
    {
    this->Internal->Storage.resize(maxDstId + 1);
    }

  // Copy data
  for (vtkIdType idIndex = 0; idIndex < numIds; ++idIndex)
    {
    this->Internal->Storage[dstIds->GetId(idIndex)] =
        array->Internal->Storage[srcIds->GetId(idIndex)];
    }

  this->DataChanged();
}

//------------------------------------------------------------------------------
void vtkUnicodeStringArray::InsertTuples(vtkIdType dstStart, vtkIdType n,
                                         vtkIdType srcStart,
                                         vtkAbstractArray *source)
{
  vtkUnicodeStringArray* sa = vtkUnicodeStringArray::SafeDownCast(source);
  if (!sa)
    {
    vtkWarningMacro("Input and outputs array data types do not match.");
    return ;
    }

  if (this->NumberOfComponents != source->GetNumberOfComponents())
    {
    vtkWarningMacro("Input and output component sizes do not match.");
    return;
    }

  vtkIdType srcEnd = srcStart + n;
  if (srcEnd > source->GetNumberOfTuples())
    {
    vtkWarningMacro("Source range exceeds array size (srcStart=" << srcStart
                    << ", n=" << n << ", numTuples="
                    << source->GetNumberOfTuples() << ").");
    return;
    }

  for (vtkIdType i = 0; i < n; ++i)
    {
    vtkIdType numComp = this->NumberOfComponents;
    vtkIdType srcLoc = (srcStart + i) * this->NumberOfComponents;
    vtkIdType dstLoc = (dstStart + i) * this->NumberOfComponents;
    while (numComp-- > 0)
      {
      this->InsertValue(dstLoc++, sa->GetValue(srcLoc++));
      }
    }

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

  this->Internal->Storage.push_back(array->Internal->Storage[j]);
  this->DataChanged();
  return static_cast<vtkIdType>(this->Internal->Storage.size()) - 1;
}

void* vtkUnicodeStringArray::GetVoidPointer(vtkIdType id)
{
  // Err.. not totally sure what to do here
  if (this->Internal->Storage.empty())
    return 0;
  else
    return &this->Internal->Storage[id];
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

  this->Internal->Storage = array->Internal->Storage;
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
  Implementation::StorageT(this->Internal->Storage).swap(this->Internal->Storage);
  this->DataChanged();
}

int vtkUnicodeStringArray::Resize(vtkIdType numTuples)
{
  this->Internal->Storage.resize(numTuples);
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
  for(Implementation::StorageT::size_type i = 0; i != this->Internal->Storage.size(); ++i)
    {
    count += static_cast<unsigned long>(this->Internal->Storage[i].byte_count());
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

vtkIdType vtkUnicodeStringArray::LookupValue(vtkVariant value)
{
  const vtkUnicodeString search_value = value.ToUnicodeString();

  for(Implementation::StorageT::size_type i = 0; i != this->Internal->Storage.size(); ++i)
    {
    if(this->Internal->Storage[i] == search_value)
      return i;
    }

  return -1;
}

void vtkUnicodeStringArray::LookupValue(vtkVariant value, vtkIdList* ids)
{
  const vtkUnicodeString search_value = value.ToUnicodeString();

  ids->Reset();
  for(Implementation::StorageT::size_type i = 0; i != this->Internal->Storage.size(); ++i)
    {
    if(this->Internal->Storage[i] == search_value)
      ids->InsertNextId(i);
    }
}

void vtkUnicodeStringArray::SetVariantValue(vtkIdType id, vtkVariant value)
{
  this->SetValue( id, value.ToUnicodeString() );
}

void vtkUnicodeStringArray::DataChanged()
{
  this->MaxId = static_cast<vtkIdType>(this->Internal->Storage.size()) - 1;
}

void vtkUnicodeStringArray::ClearLookup()
{
}

vtkIdType vtkUnicodeStringArray::InsertNextValue(const vtkUnicodeString& value)
{
  this->Internal->Storage.push_back(value);
  this->DataChanged();
  return static_cast<vtkIdType>(this->Internal->Storage.size()) - 1;
}

void vtkUnicodeStringArray::InsertValue(vtkIdType i, const vtkUnicodeString& value)
{
  // Range check
  if(static_cast<vtkIdType>(this->Internal->Storage.size()) <= i)
    this->Internal->Storage.resize(i + 1);

  this->SetValue(i, value);
}

void vtkUnicodeStringArray::SetValue(vtkIdType i, const vtkUnicodeString& value)
{
  this->Internal->Storage[i] = value;
  this->DataChanged();
}

vtkUnicodeString& vtkUnicodeStringArray::GetValue(vtkIdType i)
{
  return this->Internal->Storage[i];
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
  return this->Internal->Storage[i].utf8_str();
}

