// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSOADataArrayTemplate_txx
#define vtkSOADataArrayTemplate_txx

#ifdef VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATING
#define VTK_GDA_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#undef VTK_GDA_VALUERANGE_INSTANTIATING
#endif

#include "vtkSOADataArrayTemplate.h"

#include "vtkArrayIteratorTemplate.h"
#include "vtkBuffer.h"

#include <array>
#include <cassert>

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class ValueType>
vtkSOADataArrayTemplate<ValueType>* vtkSOADataArrayTemplate<ValueType>::New()
{
  VTK_STANDARD_NEW_BODY(vtkSOADataArrayTemplate<ValueType>);
}

//-----------------------------------------------------------------------------
#ifndef __VTK_WRAP__
template <class ValueTypeT>
vtkSOADataArrayTemplate<typename vtkSOADataArrayTemplate<ValueTypeT>::ValueType>*
vtkSOADataArrayTemplate<ValueTypeT>::FastDownCast(vtkAbstractArray* source)
{
  if (source)
  {
    switch (source->GetArrayType())
    {
      case vtkAbstractArray::SoADataArrayTemplate:
        if (vtkDataTypesCompare(source->GetDataType(), vtkTypeTraits<ValueType>::VTK_TYPE_ID))
        {
          return static_cast<vtkSOADataArrayTemplate<ValueType>*>(source);
        }
        break;
    }
  }
  return nullptr;
}
#endif

//-----------------------------------------------------------------------------
template <class ValueType>
vtkSOADataArrayTemplate<ValueType>::vtkSOADataArrayTemplate()
  : AoSData(nullptr)
  , StorageType(StorageTypeEnum::AOS)
{
  this->AoSData = vtkBuffer<ValueType>::New();
}

//-----------------------------------------------------------------------------
template <class ValueType>
vtkSOADataArrayTemplate<ValueType>::~vtkSOADataArrayTemplate()
{
  this->ClearSOAData();
  if (this->AoSData)
  {
    this->AoSData->Delete();
    this->AoSData = nullptr;
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::SetNumberOfComponents(int val)
{
  this->GenericDataArrayType::SetNumberOfComponents(val);
  size_t numComps = static_cast<size_t>(this->GetNumberOfComponents());
  assert(numComps >= 1);
  if (this->StorageType == StorageTypeEnum::SOA)
  {
    while (this->Data.size() > numComps)
    {
      this->Data.back()->Delete();
      this->Data.pop_back();
    }
    while (this->Data.size() < numComps)
    {
      this->Data.push_back(vtkBuffer<ValueType>::New());
    }
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
vtkArrayIterator* vtkSOADataArrayTemplate<ValueType>::NewIterator()
{
  vtkArrayIterator* iter = vtkArrayIteratorTemplate<ValueType>::New();
  iter->Initialize(this);
  return iter;
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::ShallowCopy(vtkDataArray* other)
{
  SelfType* o = SelfType::FastDownCast(other);
  if (o)
  {
    this->Size = o->Size;
    this->MaxId = o->MaxId;
    this->SetName(o->Name);
    this->SetNumberOfComponents(o->NumberOfComponents);
    this->CopyComponentNames(o);
    this->StorageType = o->StorageType;
    if (o->StorageType == StorageTypeEnum::SOA)
    {
      assert(this->Data.size() == o->Data.size());
      for (size_t cc = 0; cc < this->Data.size(); ++cc)
      {
        vtkBuffer<ValueType>* thisBuffer = this->Data[cc];
        vtkBuffer<ValueType>* otherBuffer = o->Data[cc];
        if (thisBuffer != otherBuffer)
        {
          thisBuffer->Delete();
          this->Data[cc] = otherBuffer;
          otherBuffer->Register(nullptr);
        }
      }
    }
    else
    {
      vtkBuffer<ValueType>* thisBuffer = this->AoSData;
      vtkBuffer<ValueType>* otherBuffer = o->AoSData;
      if (thisBuffer != otherBuffer)
      {
        thisBuffer->Delete();
        this->AoSData = otherBuffer;
        otherBuffer->Register(nullptr);
      }
    }
    this->DataChanged();
  }
  else
  {
    this->Superclass::ShallowCopy(other);
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::InsertTuples(
  vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source)
{
  // First, check for the common case of typeid(source) == typeid(this). This
  // way we don't waste time redoing the other checks in the superclass, and
  // can avoid doing a dispatch for the most common usage of this method.
  SelfType* other = vtkArrayDownCast<SelfType>(source);
  if (!other)
  {
    // Let the superclass handle dispatch/fallback.
    this->Superclass::InsertTuples(dstStart, n, srcStart, source);
    return;
  }

  if (n == 0)
  {
    return;
  }

  int numComps = this->GetNumberOfComponents();
  if (other->GetNumberOfComponents() != numComps)
  {
    vtkErrorMacro("Number of components do not match: Source: "
      << other->GetNumberOfComponents() << " Dest: " << this->GetNumberOfComponents());
    return;
  }

  vtkIdType maxSrcTupleId = srcStart + n - 1;
  vtkIdType maxDstTupleId = dstStart + n - 1;

  if (maxSrcTupleId >= other->GetNumberOfTuples())
  {
    vtkErrorMacro("Source array too small, requested tuple at index "
      << maxSrcTupleId << ", but there are only " << other->GetNumberOfTuples()
      << " tuples in the array.");
    return;
  }

  vtkIdType newSize = (maxDstTupleId + 1) * this->NumberOfComponents;
  if (this->Size < newSize)
  {
    if (!this->Resize(maxDstTupleId + 1))
    {
      vtkErrorMacro("Resize failed.");
      return;
    }
  }

  this->MaxId = std::max(this->MaxId, newSize - 1);

  if (this->StorageType == StorageTypeEnum::SOA)
  {
    for (int c = 0; c < numComps; ++c)
    {
      ValueType* srcBegin = other->GetComponentArrayPointer(c) + srcStart;
      ValueType* srcEnd = srcBegin + n;
      ValueType* dstBegin = this->GetComponentArrayPointer(c) + dstStart;
      std::copy(srcBegin, srcEnd, dstBegin);
    }
  }
  else
  {
    ValueType* target = this->AoSData->GetBuffer();
    for (vtkIdType i = srcStart; i < srcStart + n; i++)
    {
      std::vector<ValueType> values(numComps);
      other->GetTypedTuple(i, values.data());
      std::copy(values.begin(), values.end(), target + i * numComps);
    }
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::FillTypedComponent(int compIdx, ValueType value)
{
  if (this->StorageType == StorageTypeEnum::SOA)
  {
    ValueType* buffer = this->Data[compIdx]->GetBuffer();
    std::fill(buffer, buffer + this->GetNumberOfTuples(), value);
  }
  else
  {
    ValueType* buffer = this->AoSData->GetBuffer();
    int numComps = this->GetNumberOfComponents();
    for (vtkIdType i = 0; i < this->GetNumberOfTuples(); i++)
    {
      buffer[i * numComps + compIdx] = value;
    }
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::SetArray(
  int comp, ValueType* array, vtkIdType size, bool updateMaxId, bool save, int deleteMethod)
{
  const int numComps = this->GetNumberOfComponents();
  if (comp >= numComps || comp < 0)
  {
    vtkErrorMacro("Invalid component number '"
      << comp
      << "' specified. "
         "Use `SetNumberOfComponents` first to set the number of components.");
    return;
  }

  if (this->StorageType == StorageTypeEnum::AOS && this->AoSData)
  {
    this->AoSData->Delete();
    this->AoSData = nullptr;
  }

  while (this->Data.size() < static_cast<size_t>(numComps))
  {
    this->Data.push_back(vtkBuffer<ValueType>::New());
  }

  this->Data[comp]->SetBuffer(array, size);

  if (deleteMethod == VTK_DATA_ARRAY_DELETE)
  {
    this->Data[comp]->SetFreeFunction(save != 0, ::operator delete[]);
  }
  else if (deleteMethod == VTK_DATA_ARRAY_ALIGNED_FREE)
  {
#ifdef _WIN32
    this->Data[comp]->SetFreeFunction(save != 0, _aligned_free);
#else
    this->Data[comp]->SetFreeFunction(save != 0, free);
#endif
  }
  else if (deleteMethod == VTK_DATA_ARRAY_USER_DEFINED || deleteMethod == VTK_DATA_ARRAY_FREE)
  {
    this->Data[comp]->SetFreeFunction(save != 0, free);
  }

  if (updateMaxId)
  {
    this->Size = numComps * size;
    this->MaxId = this->Size - 1;
  }
  this->StorageType = StorageTypeEnum::SOA;

  this->DataChanged();
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::SetArrayFreeFunction(void (*callback)(void*))
{
  const int numComps = this->GetNumberOfComponents();
  for (int i = 0; i < numComps; ++i)
  {
    this->SetArrayFreeFunction(i, callback);
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::SetArrayFreeFunction(int comp, void (*callback)(void*))
{
  const int numComps = this->GetNumberOfComponents();
  if (comp >= numComps || comp < 0)
  {
    vtkErrorMacro("Invalid component number '"
      << comp
      << "' specified. "
         "Use `SetNumberOfComponents` first to set the number of components.");
    return;
  }
  this->Data[comp]->SetFreeFunction(false, callback);
}

//-----------------------------------------------------------------------------
template <class ValueType>
typename vtkSOADataArrayTemplate<ValueType>::ValueType*
vtkSOADataArrayTemplate<ValueType>::GetComponentArrayPointer(int comp)
{
  if (this->StorageType == StorageTypeEnum::AOS)
  {
    vtkErrorMacro("Data is currently stored in AOS mode.");
    return nullptr;
  }
  const int numComps = this->GetNumberOfComponents();
  if (comp >= numComps || comp < 0)
  {
    vtkErrorMacro("Invalid component number '" << comp << "' specified.");
    return nullptr;
  }

  return this->Data[comp]->GetBuffer();
}

//-----------------------------------------------------------------------------
template <class ValueType>
bool vtkSOADataArrayTemplate<ValueType>::AllocateTuples(vtkIdType numTuples)
{
  if (this->StorageType == StorageTypeEnum::SOA)
  {
    for (size_t cc = 0, max = this->Data.size(); cc < max; ++cc)
    {
      if (!this->Data[cc]->Allocate(numTuples))
      {
        return false;
      }
    }
  }
  else
  {
    if (!this->AoSData->Allocate(numTuples * this->GetNumberOfComponents()))
    {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
template <class ValueType>
bool vtkSOADataArrayTemplate<ValueType>::ReallocateTuples(vtkIdType numTuples)
{
  if (this->StorageType == StorageTypeEnum::SOA)
  {
    for (size_t cc = 0, max = this->Data.size(); cc < max; ++cc)
    {
      if (!this->Data[cc]->Reallocate(numTuples))
      {
        return false;
      }
    }
  }
  else
  {
    if (!this->AoSData->Reallocate(numTuples * this->GetNumberOfComponents()))
    {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
template <class ValueType>
void* vtkSOADataArrayTemplate<ValueType>::GetVoidPointer(vtkIdType valueIdx)
{
  if (this->StorageType == StorageTypeEnum::SOA)
  {
    if (this->GetNumberOfComponents() == 1)
    {
      // if there's only a single component the data will be stored in
      // contiguous memory so we can return the pointer to that array
      return static_cast<void*>(this->Data[0]->GetBuffer() + valueIdx);
    }

    // Allow warnings to be silenced:
    const char* silence = getenv("VTK_SILENCE_GET_VOID_POINTER_WARNINGS");
    if (!silence)
    {
      vtkWarningMacro(<< "GetVoidPointer called. This is very expensive for "
                         "non-array-of-structs subclasses, as the scalar array "
                         "must be generated for each call. Using the "
                         "vtkGenericDataArray API with vtkArrayDispatch are "
                         "preferred. Define the environment variable "
                         "VTK_SILENCE_GET_VOID_POINTER_WARNINGS to silence "
                         "this warning.");
    }

    size_t numValues = this->GetNumberOfValues();

    if (!this->AoSData)
    {
      this->AoSData = vtkBuffer<ValueType>::New();
    }

    if (!this->AoSData->Allocate(static_cast<vtkIdType>(numValues)))
    {
      vtkErrorMacro(<< "Error allocating a buffer of " << numValues << " '"
                    << this->GetDataTypeAsString() << "' elements.");
      return nullptr;
    }

    this->ExportToVoidPointer(static_cast<void*>(this->AoSData->GetBuffer()));
    this->ClearSOAData();
    this->StorageType = StorageTypeEnum::AOS;
  }

  return static_cast<void*>(this->AoSData->GetBuffer() + valueIdx);
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::ExportToVoidPointer(void* voidPtr)
{
  vtkIdType numTuples = this->GetNumberOfTuples();
  if (this->NumberOfComponents * numTuples == 0)
  {
    // Nothing to do.
    return;
  }

  if (!voidPtr)
  {
    vtkErrorMacro(<< "Buffer is nullptr.");
    return;
  }

  if (this->StorageType == StorageTypeEnum::SOA)
  {
    ValueType* ptr = static_cast<ValueType*>(voidPtr);
    for (vtkIdType t = 0; t < numTuples; ++t)
    {
      for (int c = 0; c < this->NumberOfComponents; ++c)
      {
        *ptr++ = this->Data[c]->GetBuffer()[t];
      }
    }
  }
  else
  {
    ValueType* buffer = this->AoSData->GetBuffer();
    std::copy(
      buffer, buffer + numTuples * this->GetNumberOfComponents(), static_cast<ValueType*>(voidPtr));
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::ClearSOAData()
{
  if (this->StorageType == StorageTypeEnum::AOS)
  {
    return;
  }
  for (size_t cc = 0; cc < this->Data.size(); ++cc)
  {
    // vtkBuffer knows the free function and whether or not to actually deallocate the memory
    this->Data[cc]->Delete();
  }
  this->Data.clear();
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::CopyData(vtkSOADataArrayTemplate<ValueType>* src)
{
  int numberOfComponents = this->GetNumberOfComponents();
  vtkIdType numberOfTuples = this->GetNumberOfTuples();
  if (numberOfComponents == 1)
  { // first optimization is if we have contiguous memory for both src and this
    ValueType* srcBegin = static_cast<ValueType*>(src->GetVoidPointer(0));
    ValueType* srcEnd = srcBegin + numberOfTuples;
    ValueType* dstBegin = static_cast<ValueType*>(this->GetVoidPointer(0));

    std::copy(srcBegin, srcEnd, dstBegin);
  }
  else if (this->StorageType == StorageTypeEnum::SOA && src->StorageType == StorageTypeEnum::SOA)
  {
    for (int comp = 0; comp < src->GetNumberOfComponents(); ++comp)
    {
      ValueType* srcBegin = src->GetComponentArrayPointer(comp);
      ValueType* srcEnd = srcBegin + numberOfTuples;
      ValueType* dstBegin = this->GetComponentArrayPointer(comp);

      std::copy(srcBegin, srcEnd, dstBegin);
    }
  }
  else if (this->StorageType == StorageTypeEnum::AOS && src->StorageType == StorageTypeEnum::AOS)
  {
    ValueType* srcBegin = src->AoSData->GetBuffer();
    ValueType* srcEnd = srcBegin + numberOfTuples * numberOfComponents;
    ValueType* dstBegin = this->AoSData->GetBuffer();

    std::copy(srcBegin, srcEnd, dstBegin);
  }
  else
  { // mismatching storage types so we'll copy data through the API
    std::vector<ValueType> tuple(numberOfComponents);
    for (vtkIdType i = 0; i < numberOfTuples; i++)
    {
      src->GetTypedTuple(i, tuple.data());
      this->SetTypedTuple(i, tuple.data());
    }
  }
}

VTK_ABI_NAMESPACE_END
#endif
