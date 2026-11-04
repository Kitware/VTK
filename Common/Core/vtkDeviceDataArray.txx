// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkDeviceDataArray_txx
#define vtkDeviceDataArray_txx

#include "vtkDeviceDataArray.h"

#include "vtkCollection.h"
#include "vtkDeviceMemory.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------------
template <typename ValueTypeT>
vtkDeviceDataArray<ValueTypeT>* vtkDeviceDataArray<ValueTypeT>::New()
{
  VTK_STANDARD_NEW_BODY(vtkDeviceDataArray<ValueTypeT>);
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
vtkDeviceDataArray<ValueTypeT>::vtkDeviceDataArray() = default;

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
vtkDeviceDataArray<ValueTypeT>::~vtkDeviceDataArray() = default;

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
bool vtkDeviceDataArray<ValueTypeT>::SetDescriptor(
  vtkMemoryDescriptor* descriptor, vtkIdType numberOfTuples, int numberOfComponents)
{
  this->Descriptor = nullptr;
  this->Space = vtkDataArray::HostMemory;
  this->DeviceValues = 0;
  this->MaxId = -1;
  this->Capacity = 0;
  this->Modified();

  if (!descriptor || numberOfTuples < 0 || numberOfComponents < 1)
  {
    vtkErrorMacro("Need a descriptor, a tuple count and at least one component.");
    return false;
  }
  vtkDataArray::MemorySpace space;
  if (!vtkDeviceMemory::SpaceFromName(descriptor->GetMemorySpace(), space))
  {
    vtkErrorMacro("Memory space '"
      << (descriptor->GetMemorySpace() ? descriptor->GetMemorySpace() : "(none)")
      << "' is not a device's.");
    return false;
  }
  const vtkIdType values = numberOfTuples * numberOfComponents;
  const vtkTypeInt64 bytes = values * static_cast<vtkTypeInt64>(sizeof(ValueType));
  if (descriptor->GetSizeInBytes() < bytes)
  {
    vtkErrorMacro("The descriptor holds " << descriptor->GetSizeInBytes() << " bytes, short of the "
                                          << bytes << " the values need.");
    return false;
  }
  const void* pointer = reinterpret_cast<const void*>(descriptor->GetPointer());
  std::string why;
  if (values > 0 && !vtkDeviceMemory::IsDevicePointer(space, pointer, &why))
  {
    vtkErrorMacro("Cannot hold " << vtkDeviceMemory::NameOf(space) << " memory: " << why);
    return false;
  }

  this->Descriptor = descriptor;
  this->Space = space;
  this->DeviceValues = values;
  this->Superclass::SetNumberOfComponents(numberOfComponents);
  this->MaxId = values - 1;
  this->Capacity = values;
  this->DataChanged();
  return true;
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
vtkDataArray::MemorySpace vtkDeviceDataArray<ValueTypeT>::GetMemorySpace()
{
  return this->Space;
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
void* vtkDeviceDataArray<ValueTypeT>::GetDeviceVoidPointer(vtkIdType valueIdx)
{
  if (!this->Descriptor)
  {
    return nullptr;
  }
  return reinterpret_cast<ValueType*>(this->Descriptor->GetPointer()) + valueIdx;
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
vtkCollection* vtkDeviceDataArray<ValueTypeT>::NewMemoryDescriptors()
{
  vtkCollection* collection = vtkCollection::New();
  if (!this->Descriptor || this->GetNumberOfValues() == 0)
  {
    return collection;
  }
  vtkMemoryDescriptor* descriptor = vtkMemoryDescriptor::New();
  descriptor->Set(this->Descriptor->GetPointer(),
    this->GetNumberOfValues() * static_cast<vtkTypeInt64>(sizeof(ValueType)),
    vtkDeviceMemory::NameOf(this->Space), "data");
  // Hold this array, and through it the memory, for as long as the
  // descriptor lives.
  descriptor->SetOwner(this);
  collection->AddItem(descriptor);
  descriptor->Delete();
  return collection;
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
void* vtkDeviceDataArray<ValueTypeT>::GetVoidPointer(vtkIdType valueIdx)
{
  return const_cast<ValueType*>(this->HostValues()) + valueIdx;
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
void vtkDeviceDataArray<ValueTypeT>::ShallowCopy(vtkDataArray* other)
{
  SelfType* same = vtkArrayDownCast<SelfType>(other);
  if (!same)
  {
    vtkErrorMacro("A device array can only share another's device memory, not copy "
      << (other ? other->GetClassName() : "nothing") << ".");
    return;
  }
  if (same == this)
  {
    return;
  }
  this->Descriptor = same->Descriptor;
  this->Space = same->Space;
  this->DeviceValues = same->DeviceValues;
  this->Superclass::SetNumberOfComponents(same->GetNumberOfComponents());
  this->MaxId = same->MaxId;
  this->Capacity = same->Capacity;
  this->DataChanged();
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
vtkTypeBool vtkDeviceDataArray<ValueTypeT>::ReserveTuples(vtkIdType numTuples)
{
  const vtkIdType values = numTuples * this->GetNumberOfComponents();
  if (values > this->DeviceValues)
  {
    vtkErrorMacro("A device array cannot grow: it holds " << this->DeviceValues << " values, and "
                                                          << values << " were asked for.");
    return 0;
  }
  this->Capacity = std::max(this->Capacity, values);
  return 1;
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
void vtkDeviceDataArray<ValueTypeT>::Modified()
{
  this->HostCurrent.store(false, std::memory_order_release);
  this->Superclass::Modified();
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
void vtkDeviceDataArray<ValueTypeT>::DataChanged()
{
  this->HostCurrent.store(false, std::memory_order_release);
  this->Superclass::DataChanged();
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
void vtkDeviceDataArray<ValueTypeT>::SetValue(vtkIdType, ValueType)
{
  this->RefuseWrite();
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
void vtkDeviceDataArray<ValueTypeT>::GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
{
  const ValueType* values = this->HostValues() + tupleIdx * this->NumberOfComponents;
  std::copy(values, values + this->NumberOfComponents, tuple);
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
void vtkDeviceDataArray<ValueTypeT>::SetTypedTuple(vtkIdType, const ValueType*)
{
  this->RefuseWrite();
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
void vtkDeviceDataArray<ValueTypeT>::SetTypedComponent(vtkIdType, int, ValueType)
{
  this->RefuseWrite();
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
bool vtkDeviceDataArray<ValueTypeT>::AllocateTuples(vtkIdType numTuples)
{
  return this->ReallocateTuples(numTuples);
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
bool vtkDeviceDataArray<ValueTypeT>::ReallocateTuples(vtkIdType numTuples)
{
  // Shrinking keeps the memory; growing past it is refused (ReserveTuples
  // says why before this is reached).
  return numTuples * this->GetNumberOfComponents() <= this->DeviceValues;
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
const typename vtkDeviceDataArray<ValueTypeT>::ValueType*
vtkDeviceDataArray<ValueTypeT>::HostValues() const
{
  if (!this->HostCurrent.load(std::memory_order_acquire))
  {
    std::scoped_lock<std::mutex> lock(this->HostMutex);
    if (!this->HostCurrent.load(std::memory_order_relaxed))
    {
      // Sized once: the array never grows, so pointers into the copy stay
      // valid when it is refreshed.
      if (this->Host.size() < static_cast<std::size_t>(this->DeviceValues))
      {
        this->Host.resize(static_cast<std::size_t>(this->DeviceValues));
      }
      std::string why;
      if (this->Descriptor &&
        !vtkDeviceMemory::CopyToHost(this->Space, this->Host.data(),
          reinterpret_cast<const void*>(this->Descriptor->GetPointer()),
          static_cast<std::size_t>(this->DeviceValues) * sizeof(ValueType), &why))
      {
        vtkErrorWithObjectMacro(const_cast<SelfType*>(this),
          "Could not read the device memory on the host: " << why << ". Reading zeros.");
        std::fill(this->Host.begin(), this->Host.end(), ValueType(0));
      }
      this->HostCurrent.store(true, std::memory_order_release);
    }
  }
  return this->Host.data();
}

//-----------------------------------------------------------------------------
template <typename ValueTypeT>
void vtkDeviceDataArray<ValueTypeT>::RefuseWrite()
{
  if (!this->WriteRefused.exchange(true))
  {
    vtkErrorMacro("A device array is read-only on the host: the write is ignored. Copy it to a "
                  "host array (NewInstance() and DeepCopy()) to change its values.");
  }
}

VTK_ABI_NAMESPACE_END
#endif
