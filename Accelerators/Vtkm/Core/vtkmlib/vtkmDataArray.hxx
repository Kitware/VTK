// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2019 Sandia Corporation.
// SPDX-FileCopyrightText: Copyright 2019 UT-Battelle, LLC.
// SPDX-FileCopyrightText: Copyright 2019 Los Alamos National Security.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-LANL-USGov
#ifndef vtkmDataArray_hxx
#define vtkmDataArray_hxx

#include "vtkObjectFactory.h"

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ArrayRangeCompute.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <cassert>

#ifndef vtkmDataArray_h
#error "vtkmDataArray.hxx should only be included from vtkmDataArray.h."
#include <vtkmDataArray.h> // For IDEs
#endif

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN

//-----------------------------------------------------------------------------
struct NotMaskValue
{
  viskores::UInt8 MaskValue;

  VISKORES_EXEC_CONT viskores::UInt8 operator()(viskores::UInt8 value) const
  {
    return static_cast<viskores::UInt8>(value != this->MaskValue);
  }
};

//-----------------------------------------------------------------------------
template <typename T>
struct ArrayHandleHelperSwapper
{
  static void SwapHelper(
    const vtkmDataArray<T>* array, std::unique_ptr<fromvtkm::ArrayHandleHelperBase<T>>& helper)
  {
    array->Helper.swap(helper);
  }

  static ArrayHandleHelperBase<T>* GetHelper(const vtkmDataArray<T>* array)
  {
    return array->Helper.get();
  }
};

//-----------------------------------------------------------------------------
template <typename T>
class ArrayHandleHelperBase
{
public:
  using ValueType = T;

  ArrayHandleHelperBase(const viskores::cont::UnknownArrayHandle& vtkmArray)
    : VtkmArray(vtkmArray)
  {
    assert(vtkmArray.IsBaseComponentType<T>());
  }

  virtual ~ArrayHandleHelperBase() = default;

  viskores::IdComponent GetNumberOfComponents() const
  {
    return this->VtkmArray.GetNumberOfComponentsFlat();
  }

  void Reallocate(const vtkmDataArray<T>* self, viskores::Id numberOfTuples)
  {
    this->VtkmArray.Allocate(numberOfTuples, viskores::CopyFlag::On);
    this->ResetHelper(self);
  }

  bool ComputeScalarRange(const vtkmDataArray<T>* self, double* ranges, const unsigned char* ghosts,
    viskores::UInt8 ghostValueToSkip, bool finitesOnly);
  bool ComputeVectorRange(const vtkmDataArray<T>* self, double range[2],
    const unsigned char* ghosts, viskores::UInt8 ghostValueToSkip, bool finitesOnly);

  viskores::cont::UnknownArrayHandle GetArrayHandle() const { return this->VtkmArray; }

  virtual void GetTuple(
    const vtkmDataArray<T>* self, viskores::Id valIdx, ValueType* values) const = 0;
  virtual void SetTuple(
    const vtkmDataArray<T>* self, viskores::Id valIdx, const ValueType* values) = 0;
  virtual ValueType GetComponent(
    const vtkmDataArray<T>* self, viskores::Id valIdx, viskores::IdComponent compIdx) const = 0;
  virtual void SetComponent(const vtkmDataArray<T>* self, viskores::Id valIdx,
    viskores::IdComponent compIdx, const ValueType& value) = 0;

protected:
  viskores::cont::UnknownArrayHandle VtkmArray;

private:
  // Some operations might invalidate portals and other information pulled from the ArrayHandle,
  // so reset the helper registered with the vtkmDataArray so that they get re-pulled if necessary.
  void ResetHelper(const vtkmDataArray<T>* self);
};

template <typename T>
class ArrayHandleHelperUnknown : public ArrayHandleHelperBase<T>
{
public:
  ArrayHandleHelperUnknown(const viskores::cont::UnknownArrayHandle& array)
    : ArrayHandleHelperBase<T>(array)
  {
  }

  void GetTuple(const vtkmDataArray<T>* self, viskores::Id valIdx, T* values) const override;
  void SetTuple(const vtkmDataArray<T>* self, viskores::Id valIdx, const T* values) override;
  T GetComponent(const vtkmDataArray<T>* self, viskores::Id valIdx,
    viskores::IdComponent compIdx) const override;
  void SetComponent(const vtkmDataArray<T>* self, viskores::Id valIdx,
    viskores::IdComponent compIdx, const T& value) override;

private:
  ArrayHandleHelperBase<T>* SwapReadHelper(const vtkmDataArray<T>* self) const;
  ArrayHandleHelperBase<T>* SwapWriteHelper(const vtkmDataArray<T>* self) const;
};

template <typename T>
std::unique_ptr<ArrayHandleHelperBase<T>> MakeArrayHandleHelperUnknown(
  const viskores::cont::UnknownArrayHandle& array)
{
  return std::unique_ptr<ArrayHandleHelperBase<T>>{ new ArrayHandleHelperUnknown<T>(array) };
}

template <typename ArrayHandleType>
class ArrayHandleHelperWrite
  : public ArrayHandleHelperBase<typename ArrayHandleType::ValueType::ComponentType>
{
  using T = typename ArrayHandleType::ValueType::ComponentType;

public:
  ArrayHandleHelperWrite(const viskores::cont::UnknownArrayHandle& array);

  void GetTuple(const vtkmDataArray<T>* self, viskores::Id valIdx, T* values) const override;
  void SetTuple(const vtkmDataArray<T>* self, viskores::Id valIdx, const T* values) override;
  T GetComponent(const vtkmDataArray<T>* self, viskores::Id valIdx,
    viskores::IdComponent compIdx) const override;
  void SetComponent(const vtkmDataArray<T>* self, viskores::Id valIdx,
    viskores::IdComponent compIdx, const T& value) override;

private:
  ArrayHandleType TypedArray;
  typename ArrayHandleType::WritePortalType WritePortal;
};

template <typename ArrayHandleType>
std::unique_ptr<ArrayHandleHelperBase<typename ArrayHandleType::ValueType::ComponentType>>
MakeArrayHandleHelperWrite(const ArrayHandleType& array)
{
  return std::unique_ptr<ArrayHandleHelperBase<typename ArrayHandleType::ValueType::ComponentType>>{
    new ArrayHandleHelperWrite<ArrayHandleType>(array)
  };
}

template <typename ArrayHandleType>
class ArrayHandleHelperRead
  : public ArrayHandleHelperBase<typename ArrayHandleType::ValueType::ComponentType>
{
  using T = typename ArrayHandleType::ValueType::ComponentType;

public:
  ArrayHandleHelperRead(const viskores::cont::UnknownArrayHandle& array);

  void GetTuple(const vtkmDataArray<T>* self, viskores::Id valIdx, T* values) const override;
  void SetTuple(const vtkmDataArray<T>* self, viskores::Id valIdx, const T* values) override;
  T GetComponent(const vtkmDataArray<T>* self, viskores::Id valIdx,
    viskores::IdComponent compIdx) const override;
  void SetComponent(const vtkmDataArray<T>* self, viskores::Id valIdx,
    viskores::IdComponent compIdx, const T& value) override;

private:
  ArrayHandleType TypedArray;
  typename ArrayHandleType::ReadPortalType ReadPortal;
};

template <typename ArrayHandleType>
std::unique_ptr<ArrayHandleHelperBase<typename ArrayHandleType::ValueType::ComponentType>>
MakeArrayHandleHelperRead(const ArrayHandleType& array)
{
  return std::unique_ptr<ArrayHandleHelperBase<typename ArrayHandleType::ValueType::ComponentType>>{
    new ArrayHandleHelperRead<ArrayHandleType>(array)
  };
}

//-----------------------------------------------------------------------------
template <typename T>
bool ArrayHandleHelperBase<T>::ComputeScalarRange(const vtkmDataArray<T>* self, double* ranges,
  const unsigned char* ghosts, viskores::UInt8 ghostValueToSkip, bool finitesOnly)
{
  if (this->VtkmArray.GetNumberOfValues() < 1)
  {
    for (int i = 0; i < this->GetNumberOfComponents(); ++i)
    {
      ranges[(2 * i) + 0] = VTK_DOUBLE_MAX;
      ranges[(2 * i) + 1] = VTK_DOUBLE_MIN;
    }
    return false;
  }

  viskores::cont::ArrayHandle<viskores::UInt8> ghostArray;
  if (ghosts)
  {
    ghostArray = viskores::cont::make_ArrayHandle(
      ghosts, this->VtkmArray.GetNumberOfValues(), viskores::CopyFlag::Off);
    if (ghostValueToSkip != 0)
    {
      auto transform =
        viskores::cont::make_ArrayHandleTransform(ghostArray, NotMaskValue{ ghostValueToSkip });
      viskores::cont::ArrayHandle<viskores::UInt8> newGhostArray;
      viskores::cont::ArrayCopy(transform, newGhostArray);
      ghostArray = newGhostArray;
    }
  }

  viskores::cont::ArrayHandle<viskores::Range> rangeArray =
    viskores::cont::ArrayRangeCompute(this->VtkmArray, ghostArray, finitesOnly);

  auto rangePortal = rangeArray.ReadPortal();
  for (viskores::IdComponent index = 0; index < rangePortal.GetNumberOfValues(); ++index)
  {
    viskores::Range r = rangePortal.Get(index);
    ranges[(2 * index) + 0] = r.Min;
    ranges[(2 * index) + 1] = r.Max;
  }

  this->ResetHelper(self);
  return true;
}

template <typename T>
bool ArrayHandleHelperBase<T>::ComputeVectorRange(const vtkmDataArray<T>* self, double range[2],
  const unsigned char* ghosts, viskores::UInt8 ghostValueToSkip, bool finitesOnly)
{
  if (this->VtkmArray.GetNumberOfValues() < 1)
  {
    range[0] = VTK_DOUBLE_MAX;
    range[1] = VTK_DOUBLE_MIN;
    return false;
  }

  viskores::cont::ArrayHandle<viskores::UInt8> ghostArray;
  if (ghosts)
  {
    ghostArray = viskores::cont::make_ArrayHandle(
      ghosts, this->VtkmArray.GetNumberOfValues(), viskores::CopyFlag::Off);
    if (ghostValueToSkip != 0)
    {
      auto transform =
        viskores::cont::make_ArrayHandleTransform(ghostArray, NotMaskValue{ ghostValueToSkip });
      viskores::cont::ArrayHandle<viskores::UInt8> newGhostArray;
      viskores::cont::ArrayCopy(transform, newGhostArray);
      ghostArray = newGhostArray;
    }
  }

  viskores::Range r =
    viskores::cont::ArrayRangeComputeMagnitude(this->VtkmArray, ghostArray, finitesOnly);

  range[0] = r.Min;
  range[1] = r.Max;

  this->ResetHelper(self);
  return true;
}

template <typename T>
void ArrayHandleHelperBase<T>::ResetHelper(const vtkmDataArray<T>* self)
{
  auto newHelper = MakeArrayHandleHelperUnknown<T>(this->VtkmArray);
  ArrayHandleHelperSwapper<T>::SwapHelper(self, newHelper);
}

//-----------------------------------------------------------------------------
template <typename T>
void ArrayHandleHelperUnknown<T>::GetTuple(
  const vtkmDataArray<T>* self, viskores::Id valIdx, T* values) const
{
  this->SwapReadHelper(self)->GetTuple(self, valIdx, values);
}

template <typename T>
void ArrayHandleHelperUnknown<T>::SetTuple(
  const vtkmDataArray<T>* self, viskores::Id valIdx, const T* values)
{
  this->SwapWriteHelper(self)->SetTuple(self, valIdx, values);
}

template <typename T>
T ArrayHandleHelperUnknown<T>::GetComponent(
  const vtkmDataArray<T>* self, viskores::Id valIdx, viskores::IdComponent compIdx) const
{
  return this->SwapReadHelper(self)->GetComponent(self, valIdx, compIdx);
}

template <typename T>
void ArrayHandleHelperUnknown<T>::SetComponent(
  const vtkmDataArray<T>* self, viskores::Id valIdx, viskores::IdComponent compIdx, const T& value)
{
  this->SwapWriteHelper(self)->SetComponent(self, valIdx, compIdx, value);
}

template <typename T>
ArrayHandleHelperBase<T>* ArrayHandleHelperUnknown<T>::SwapReadHelper(
  const vtkmDataArray<T>* self) const
{
  std::unique_ptr<ArrayHandleHelperBase<T>> newHelper;
  if (this->VtkmArray.template CanConvert<viskores::cont::ArrayHandleRuntimeVec<T>>())
  {
    newHelper = MakeArrayHandleHelperRead(
      this->VtkmArray.template AsArrayHandle<viskores::cont::ArrayHandleRuntimeVec<T>>());
  }
  else
  {
    newHelper = MakeArrayHandleHelperRead(this->VtkmArray.template ExtractArrayFromComponents<T>());
  }
  ArrayHandleHelperSwapper<T>::SwapHelper(self, newHelper);
  return ArrayHandleHelperSwapper<T>::GetHelper(self);
}

template <typename T>
ArrayHandleHelperBase<T>* ArrayHandleHelperUnknown<T>::SwapWriteHelper(
  const vtkmDataArray<T>* self) const
{
  std::unique_ptr<ArrayHandleHelperBase<T>> newHelper;
  if (this->VtkmArray.template CanConvert<viskores::cont::ArrayHandleRuntimeVec<T>>())
  {
    newHelper = MakeArrayHandleHelperWrite(
      this->VtkmArray.template AsArrayHandle<viskores::cont::ArrayHandleRuntimeVec<T>>());
  }
  else
  {
    newHelper =
      MakeArrayHandleHelperWrite(this->VtkmArray.template ExtractArrayFromComponents<T>());
  }
  ArrayHandleHelperSwapper<T>::SwapHelper(self, newHelper);
  return ArrayHandleHelperSwapper<T>::GetHelper(self);
}

//-----------------------------------------------------------------------------
template <typename ArrayHandleType>
ArrayHandleHelperWrite<ArrayHandleType>::ArrayHandleHelperWrite(
  const viskores::cont::UnknownArrayHandle& array)
  : ArrayHandleHelperBase<T>(array)
  , TypedArray(array.AsArrayHandle<ArrayHandleType>())
  , WritePortal(TypedArray.WritePortal())
{
}

template <typename ArrayHandleType>
void ArrayHandleHelperWrite<ArrayHandleType>::GetTuple(
  const vtkmDataArray<T>*, viskores::Id valIdx, T* values) const
{
  auto tuple = this->WritePortal.Get(valIdx);
  for (viskores::IdComponent cIndex = 0; cIndex < tuple.GetNumberOfComponents(); ++cIndex)
  {
    values[cIndex] = tuple[cIndex];
  }
}

template <typename ArrayHandleType>
void ArrayHandleHelperWrite<ArrayHandleType>::SetTuple(
  const vtkmDataArray<T>* vtkNotUsed(self), viskores::Id valIdx, const T* values)
{
  // It's a little weird to get a value to set it, but these arrays with variable length Vecs
  // actually return a reference back to the array, so you are actually just setting values
  // into the array.
  auto tuple = this->WritePortal.Get(valIdx);
  for (viskores::IdComponent cIndex = 0; cIndex < tuple.GetNumberOfComponents(); ++cIndex)
  {
    tuple[cIndex] = values[cIndex];
  }
  this->WritePortal.Set(valIdx, tuple);
}

template <typename ArrayHandleType>
auto ArrayHandleHelperWrite<ArrayHandleType>::GetComponent(const vtkmDataArray<T>* vtkNotUsed(self),
  viskores::Id valIdx, viskores::IdComponent compIdx) const -> T
{
  return this->WritePortal.Get(valIdx)[compIdx];
}

template <typename ArrayHandleType>
void ArrayHandleHelperWrite<ArrayHandleType>::SetComponent(const vtkmDataArray<T>* vtkNotUsed(self),
  viskores::Id valIdx, viskores::IdComponent compIdx, const T& value)
{
  auto tuple = this->WritePortal.Get(valIdx);
  tuple[compIdx] = value;
  this->WritePortal.Set(valIdx, tuple);
}

//-----------------------------------------------------------------------------
// The write helper does all that the read helper does and more. However, we have a separate
// read helper because the construction of the write helper will delete any data on the
// device, so we have a read subset that allows leaving the data on the device.
template <typename ArrayHandleType>
ArrayHandleHelperRead<ArrayHandleType>::ArrayHandleHelperRead(
  const viskores::cont::UnknownArrayHandle& array)
  : ArrayHandleHelperBase<T>(array)
  , TypedArray(array.AsArrayHandle<ArrayHandleType>())
  , ReadPortal(TypedArray.ReadPortal())
{
}

template <typename ArrayHandleType>
void ArrayHandleHelperRead<ArrayHandleType>::GetTuple(
  const vtkmDataArray<T>*, viskores::Id valIdx, T* values) const
{
  auto tuple = this->ReadPortal.Get(valIdx);
  for (viskores::IdComponent cIndex = 0; cIndex < tuple.GetNumberOfComponents(); ++cIndex)
  {
    values[cIndex] = tuple[cIndex];
  }
}

template <typename ArrayHandleType>
void ArrayHandleHelperRead<ArrayHandleType>::SetTuple(
  const vtkmDataArray<T>* self, viskores::Id valIdx, const T* values)
{
  auto helper = MakeArrayHandleHelperWrite(this->TypedArray);
  ArrayHandleHelperSwapper<T>::SwapHelper(self, helper);
  ArrayHandleHelperSwapper<T>::GetHelper(self)->SetTuple(self, valIdx, values);
}

template <typename ArrayHandleType>
auto ArrayHandleHelperRead<ArrayHandleType>::GetComponent(const vtkmDataArray<T>* vtkNotUsed(self),
  viskores::Id valIdx, viskores::IdComponent compIdx) const -> T
{
  return this->ReadPortal.Get(valIdx)[compIdx];
}

template <typename ArrayHandleType>
void ArrayHandleHelperRead<ArrayHandleType>::SetComponent(
  const vtkmDataArray<T>* self, viskores::Id valIdx, viskores::IdComponent compIdx, const T& value)
{
  auto helper = MakeArrayHandleHelperWrite(this->TypedArray);
  ArrayHandleHelperSwapper<T>::SwapHelper(self, helper);
  ArrayHandleHelperSwapper<T>::GetHelper(self)->SetComponent(self, valIdx, compIdx, value);
}

VTK_ABI_NAMESPACE_END
} // fromvtkm

VTK_ABI_NAMESPACE_BEGIN
//=============================================================================
template <typename T>
vtkmDataArray<T>::vtkmDataArray() = default;

template <typename T>
vtkmDataArray<T>::~vtkmDataArray() = default;

template <typename T>
vtkmDataArray<T>* vtkmDataArray<T>::New()
{
  VTK_STANDARD_NEW_BODY(vtkmDataArray<T>);
}

template <typename T>
void vtkmDataArray<T>::SetVtkmArrayHandle(const viskores::cont::UnknownArrayHandle& ah)
{
  this->Helper = fromvtkm::MakeArrayHandleHelperUnknown<T>(ah);

  this->SetNumberOfComponents(this->Helper->GetNumberOfComponents());
  this->Size = ah.GetNumberOfValues() * this->GetNumberOfComponents();
  this->MaxId = this->Size - 1;
}

template <typename T>
viskores::cont::UnknownArrayHandle vtkmDataArray<T>::GetVtkmUnknownArrayHandle() const
{
  if (this->Helper)
  {
    return this->Helper->GetArrayHandle();
  }
  return {};
}

//-----------------------------------------------------------------------------
template <typename T>
void* vtkmDataArray<T>::GetVoidPointer(vtkIdType valueIdx)
{
  viskores::cont::ArrayHandleRuntimeVec<T> array{ this->GetNumberOfComponents() };
  if (this->GetVtkmUnknownArrayHandle().template CanConvert<decltype(array)>())
  {
    this->GetVtkmUnknownArrayHandle().AsArrayHandle(array);
  }
  else
  {
    // Data does not appear to be in a basic layout. Copy array.
    viskores::cont::ArrayCopy(this->GetVtkmUnknownArrayHandle(), array);
    this->SetVtkmArrayHandle(array);
  }

  // Get the write pointer to the data (since there is no way to know whether
  // this array will be written to).
  T* pointer = array.GetComponentsArray().GetWritePointer();
  return &(pointer[valueIdx]);
}

template <typename T>
void* vtkmDataArray<T>::WriteVoidPointer(vtkIdType valueIdx, vtkIdType numValues)
{
  vtkIdType numTuples = (numValues + this->NumberOfComponents - 1) / this->NumberOfComponents;
  this->ReallocateTuples(numTuples);
  return this->GetVoidPointer(valueIdx);
}

//-----------------------------------------------------------------------------
template <typename T>
auto vtkmDataArray<T>::GetValue(vtkIdType valueIdx) const -> ValueType
{
  assert(this->Helper);
  auto idx = valueIdx / this->GetNumberOfComponents();
  auto comp = valueIdx % this->GetNumberOfComponents();
  return this->Helper->GetComponent(this, idx, comp);
}

template <typename T>
void vtkmDataArray<T>::SetValue(vtkIdType valueIdx, ValueType value)
{
  assert(this->Helper);

  auto idx = valueIdx / this->GetNumberOfComponents();
  auto comp = valueIdx % this->GetNumberOfComponents();
  try
  {
    this->Helper->SetComponent(this, idx, comp, value);
  }
  catch (viskores::cont::Error)
  {
    vtkErrorMacro(<< "Underlying ArrayHandle (" << this->Helper->GetArrayHandle().GetArrayTypeName()
                  << ") does not support writes through vtkmDataArray");
  }
}

template <typename T>
void vtkmDataArray<T>::GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
{
  assert(this->Helper);
  this->Helper->GetTuple(this, tupleIdx, tuple);
}

template <typename T>
void vtkmDataArray<T>::SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
{
  assert(this->Helper);

  try
  {
    this->Helper->SetTuple(this, tupleIdx, tuple);
  }
  catch (viskores::cont::Error)
  {
    vtkErrorMacro(<< "Underlying ArrayHandle (" << this->Helper->GetArrayHandle().GetArrayTypeName()
                  << ") is read-only");
  }
}

template <typename T>
auto vtkmDataArray<T>::GetTypedComponent(vtkIdType tupleIdx, int compIdx) const -> ValueType
{
  assert(this->Helper);
  return this->Helper->GetComponent(this, tupleIdx, compIdx);
}

template <typename T>
void vtkmDataArray<T>::SetTypedComponent(vtkIdType tupleIdx, int compIdx, ValueType value)
{
  assert(this->Helper);

  try
  {
    this->Helper->SetComponent(this, tupleIdx, compIdx, value);
  }
  catch (viskores::cont::Error)
  {
    vtkErrorMacro(<< "Underlying ArrayHandle (" << this->Helper->GetArrayHandle().GetArrayTypeName()
                  << ") is read-only");
  }
}

//-----------------------------------------------------------------------------
template <typename T>
bool vtkmDataArray<T>::ComputeScalarRange(
  double* ranges, const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  if (this->Helper)
  {
    return this->Helper->ComputeScalarRange(this, ranges, ghosts, ghostsToSkip, false);
  }
  return false;
}

template <typename T>
bool vtkmDataArray<T>::ComputeVectorRange(
  double range[2], const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  if (this->Helper)
  {
    return this->Helper->ComputeVectorRange(this, range, ghosts, ghostsToSkip, false);
  }
  return false;
}

template <typename T>
bool vtkmDataArray<T>::ComputeFiniteScalarRange(
  double* ranges, const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  if (this->Helper)
  {
    return this->Helper->ComputeScalarRange(this, ranges, ghosts, ghostsToSkip, true);
  }
  return false;
}

template <typename T>
bool vtkmDataArray<T>::ComputeFiniteVectorRange(
  double range[2], const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  if (this->Helper)
  {
    return this->Helper->ComputeVectorRange(this, range, ghosts, ghostsToSkip, true);
  }
  return false;
}

//-----------------------------------------------------------------------------
template <typename T>
bool vtkmDataArray<T>::AllocateTuples(vtkIdType numberOfTuples)
{
  viskores::cont::ArrayHandleRuntimeVec<T> arrayHandle(this->NumberOfComponents);
  arrayHandle.Allocate(numberOfTuples);
  // Reset helper since any held portals have been invalidated.
  this->Helper = fromvtkm::MakeArrayHandleHelperUnknown<T>(arrayHandle);

  // Size and MaxId are updated by the caller
  return true;
}

template <typename T>
bool vtkmDataArray<T>::ReallocateTuples(vtkIdType numberOfTuples)
{
  if (this->Helper)
  {
    // Size and MaxId are updated by the caller
    try
    {
      this->Helper->Reallocate(this, numberOfTuples);
      return true;
    }
    catch (viskores::cont::Error)
    {
      vtkErrorMacro(<< "Underlying ArrayHandle ("
                    << this->Helper->GetArrayHandle().GetArrayTypeName()
                    << ") does not support reallocation through vtkmDataArray");
      return false;
    }
  }
  return false;
}

VTK_ABI_NAMESPACE_END
#endif // vtkmDataArray_hxx
