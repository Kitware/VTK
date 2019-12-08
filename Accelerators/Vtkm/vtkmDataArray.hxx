//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 Sandia Corporation.
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkmDataArray_hxx
#define vtkmDataArray_hxx

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayRangeCompute.h>

namespace internal
{
//=============================================================================
template <typename T>
class ArrayHandleWrapperBase
{
public:
  virtual ~ArrayHandleWrapperBase() = default;

  virtual vtkIdType GetNumberOfTuples() const = 0;
  virtual int GetNumberOfComponents() const = 0;

  virtual void SetTuple(vtkIdType idx, const T* value) = 0;
  virtual void GetTuple(vtkIdType idx, T* value) const = 0;

  virtual void SetComponent(vtkIdType tuple, int comp, const T& value) = 0;
  virtual T GetComponent(vtkIdType tuple, int comp) const = 0;

  virtual void Allocate(vtkIdType numTuples) = 0;
  virtual void Reallocate(vtkIdType numTuples) = 0;

  virtual vtkm::cont::VariantArrayHandle GetVtkmVariantArrayHandle() const = 0;
};

//-----------------------------------------------------------------------------
template <typename T,
  typename NumComponentsTags = typename vtkm::VecTraits<T>::HasMultipleComponents>
struct FlattenVec;

template <typename T>
struct FlattenVec<T, vtkm::VecTraitsTagMultipleComponents>
{
  using SubVec = FlattenVec<typename vtkm::VecTraits<T>::ComponentType>;
  using ComponentType = typename SubVec::ComponentType;

  static VTKM_EXEC_CONT vtkm::IdComponent GetNumberOfComponents(const T& vec)
  {
    return vtkm::VecTraits<T>::GetNumberOfComponents(vec) *
      SubVec::GetNumberOfComponents(vtkm::VecTraits<T>::GetComponent(vec, 0));
  }

  static VTKM_EXEC_CONT const ComponentType& GetComponent(const T& vec, vtkm::IdComponent comp)
  {
    auto ncomps = SubVec::GetNumberOfComponents(vtkm::VecTraits<T>::GetComponent(vec, 0));
    return SubVec::GetComponent(
      vtkm::VecTraits<T>::GetComponent(vec, comp / ncomps), comp % ncomps);
  }

  static VTKM_EXEC_CONT ComponentType& GetComponent(T& vec, vtkm::IdComponent comp)
  {
    auto ncomps = SubVec::GetNumberOfComponents(vtkm::VecTraits<T>::GetComponent(vec, 0));
    return SubVec::GetComponent(
      vtkm::VecTraits<T>::GetComponent(vec, comp / ncomps), comp % ncomps);
  }
};

template <typename T>
struct FlattenVec<T, vtkm::VecTraitsTagSingleComponent>
{
  using ComponentType = typename vtkm::VecTraits<T>::ComponentType;

  static constexpr VTKM_EXEC_CONT vtkm::IdComponent GetNumberOfComponents(const T&) { return 1; }

  static VTKM_EXEC_CONT const ComponentType& GetComponent(const T& vec, vtkm::IdComponent)
  {
    return vtkm::VecTraits<T>::GetComponent(vec, 0);
  }

  static VTKM_EXEC_CONT ComponentType& GetComponent(T& vec, vtkm::IdComponent)
  {
    return vtkm::VecTraits<T>::GetComponent(vec, 0);
  }
};

//-----------------------------------------------------------------------------
template <typename ValueType, typename StorageTag>
class ArrayHandleWrapper
  : public ArrayHandleWrapperBase<typename FlattenVec<ValueType>::ComponentType>
{
private:
  using ArrayHandleType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
  using ComponentType = typename FlattenVec<ValueType>::ComponentType;
  using PortalType = typename ArrayHandleType::PortalControl;

public:
  explicit ArrayHandleWrapper(const ArrayHandleType& handle)
    : Handle(handle)
  {
    this->Portal = this->Handle.GetPortalControl();
    this->NumberOfComponents = (this->Portal.GetNumberOfValues() == 0)
      ? 1
      : FlattenVec<ValueType>::GetNumberOfComponents(this->Portal.Get(0));
  }

  vtkIdType GetNumberOfTuples() const override { return this->Portal.GetNumberOfValues(); }

  int GetNumberOfComponents() const override { return this->NumberOfComponents; }

  void SetTuple(vtkIdType idx, const ComponentType* value) override
  {
    // some vector types are not default constructible...
    auto v = this->Portal.Get(idx);
    for (vtkm::IdComponent i = 0; i < this->NumberOfComponents; ++i)
    {
      FlattenVec<ValueType>::GetComponent(v, i) = value[i];
    }
    this->Portal.Set(idx, v);
  }

  void GetTuple(vtkIdType idx, ComponentType* value) const override
  {
    auto v = this->Portal.Get(idx);
    for (vtkm::IdComponent i = 0; i < this->NumberOfComponents; ++i)
    {
      value[i] = FlattenVec<ValueType>::GetComponent(v, i);
    }
  }

  void SetComponent(vtkIdType tuple, int comp, const ComponentType& value) override
  {
    auto v = this->Portal.Get(tuple);
    FlattenVec<ValueType>::GetComponent(v, comp) = value;
    this->Portal.Set(tuple, v);
  }

  ComponentType GetComponent(vtkIdType tuple, int comp) const override
  {
    return FlattenVec<ValueType>::GetComponent(this->Portal.Get(tuple), comp);
  }

  void Allocate(vtkIdType numTuples) override
  {
    this->Handle.Allocate(numTuples);
    this->Portal = this->Handle.GetPortalControl();
  }

  void Reallocate(vtkIdType numTuples) override
  {
    ArrayHandleType newHandle;
    newHandle.Allocate(numTuples);
    vtkm::cont::Algorithm::CopySubRange(this->Handle, 0,
      std::min(this->Handle.GetNumberOfValues(), newHandle.GetNumberOfValues()), newHandle, 0);
    this->Handle = std::move(newHandle);
    this->Portal = this->Handle.GetPortalControl();
  }

  vtkm::cont::VariantArrayHandle GetVtkmVariantArrayHandle() const override
  {
    return vtkm::cont::VariantArrayHandle{ this->Handle };
  }

private:
  ArrayHandleType Handle;
  PortalType Portal;
  vtkm::IdComponent NumberOfComponents;
};

//-----------------------------------------------------------------------------
template <typename ValueType, typename StorageTag>
class ArrayHandleWrapperReadOnly
  : public ArrayHandleWrapperBase<typename FlattenVec<ValueType>::ComponentType>
{
private:
  using ArrayHandleType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
  using ComponentType = typename FlattenVec<ValueType>::ComponentType;
  using PortalType = typename ArrayHandleType::PortalConstControl;

public:
  explicit ArrayHandleWrapperReadOnly(const ArrayHandleType& handle)
    : Handle(handle)
  {
    this->Portal = this->Handle.GetPortalConstControl();
    this->NumberOfComponents = (this->Portal.GetNumberOfValues() == 0)
      ? 1
      : FlattenVec<ValueType>::GetNumberOfComponents(this->Portal.Get(0));
  }

  vtkIdType GetNumberOfTuples() const override { return this->Portal.GetNumberOfValues(); }

  int GetNumberOfComponents() const override { return this->NumberOfComponents; }

  void SetTuple(vtkIdType, const ComponentType*) override
  {
    vtkGenericWarningMacro(<< "SetTuple called on read-only vtkmDataArray");
  }

  void GetTuple(vtkIdType idx, ComponentType* value) const override
  {
    auto v = this->Portal.Get(idx);
    for (vtkm::IdComponent i = 0; i < this->NumberOfComponents; ++i)
    {
      value[i] = FlattenVec<ValueType>::GetComponent(v, i);
    }
  }

  void SetComponent(vtkIdType, int, const ComponentType&) override
  {
    vtkGenericWarningMacro(<< "SetComponent called on read-only vtkmDataArray");
  }

  ComponentType GetComponent(vtkIdType tuple, int comp) const override
  {
    return FlattenVec<ValueType>::GetComponent(this->Portal.Get(tuple), comp);
  }

  void Allocate(vtkIdType) override
  {
    vtkGenericWarningMacro(<< "Allocate called on read-only vtkmDataArray");
  }

  void Reallocate(vtkIdType) override
  {
    vtkGenericWarningMacro(<< "Reallocate called on read-only vtkmDataArray");
  }

  vtkm::cont::VariantArrayHandle GetVtkmVariantArrayHandle() const override
  {
    return vtkm::cont::VariantArrayHandle{ this->Handle };
  }

private:
  ArrayHandleType Handle;
  PortalType Portal;
  vtkm::IdComponent NumberOfComponents;
};

//-----------------------------------------------------------------------------
template <typename T>
class ArrayHandleWrapperFlatSOA : public ArrayHandleWrapperBase<T>
{
private:
  using ArrayHandleType = vtkm::cont::ArrayHandle<T>;
  using PortalType = typename ArrayHandleType::PortalControl;
  using VtkmArrayType = vtkm::cont::ArrayHandleGroupVecVariable<ArrayHandleType,
    vtkm::cont::ArrayHandleCounting<vtkm::Id> >;

public:
  explicit ArrayHandleWrapperFlatSOA(const ArrayHandleType& handle, int numberOfComponents)
    : Handle(handle)
    , NumberOfComponents(numberOfComponents)
  {
    this->Portal = this->Handle.GetPortalControl();
  }

  vtkIdType GetNumberOfTuples() const override
  {
    return this->Portal.GetNumberOfValues() / this->NumberOfComponents;
  }

  int GetNumberOfComponents() const override { return this->NumberOfComponents; }

  void SetTuple(vtkIdType idx, const T* value) override
  {
    vtkm::Id start = idx * this->NumberOfComponents;
    vtkm::Id end = start + this->NumberOfComponents;
    for (auto i = start; i < end; ++i)
    {
      this->Portal.Set(i, *value++);
    }
  }

  void GetTuple(vtkIdType idx, T* value) const override
  {
    vtkm::Id start = idx * this->NumberOfComponents;
    vtkm::Id end = start + this->NumberOfComponents;
    for (auto i = start; i < end; ++i)
    {
      *value++ = this->Portal.Get(i);
    }
  }

  void SetComponent(vtkIdType tuple, int comp, const T& value) override
  {
    this->Portal.Set((tuple * this->NumberOfComponents) + comp, value);
  }

  T GetComponent(vtkIdType tuple, int comp) const override
  {
    return this->Portal.Get((tuple * this->NumberOfComponents) + comp);
  }

  void Allocate(vtkIdType numTuples) override
  {
    this->Handle.Allocate(numTuples * this->NumberOfComponents);
    this->Portal = this->Handle.GetPortalControl();
  }

  void Reallocate(vtkIdType numTuples) override
  {
    ArrayHandleType newHandle;
    newHandle.Allocate(numTuples * this->NumberOfComponents);
    vtkm::cont::Algorithm::CopySubRange(this->Handle, 0,
      std::min(this->Handle.GetNumberOfValues(), newHandle.GetNumberOfValues()), newHandle, 0);
    this->Handle = std::move(newHandle);
    this->Portal = this->Handle.GetPortalControl();
  }

  vtkm::cont::VariantArrayHandle GetVtkmVariantArrayHandle() const override
  {
    return vtkm::cont::VariantArrayHandle{ this->GetVtkmArray() };
  }

private:
  VtkmArrayType GetVtkmArray() const
  {
    auto length = this->Handle.GetNumberOfValues() / this->NumberOfComponents;
    auto offsets = vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, this->NumberOfComponents, length);
    return VtkmArrayType{ this->Handle, offsets };
  }

  ArrayHandleType Handle;
  PortalType Portal;
  vtkm::IdComponent NumberOfComponents;
};

//-----------------------------------------------------------------------------
template <typename ArrayHandleType>
using IsReadOnly = std::integral_constant<bool,
  !vtkm::cont::internal::IsWritableArrayHandle<ArrayHandleType>::value>;

template <typename T, typename S>
ArrayHandleWrapperBase<typename FlattenVec<T>::ComponentType>* WrapArrayHandle(
  const vtkm::cont::ArrayHandle<T, S>& ah, std::false_type)
{
  return new ArrayHandleWrapper<T, S>{ ah };
}

template <typename T, typename S>
ArrayHandleWrapperBase<typename FlattenVec<T>::ComponentType>* WrapArrayHandle(
  const vtkm::cont::ArrayHandle<T, S>& ah, std::true_type)
{
  return new ArrayHandleWrapperReadOnly<T, S>{ ah };
}

template <typename T, typename S>
ArrayHandleWrapperBase<typename FlattenVec<T>::ComponentType>* MakeArrayHandleWrapper(
  const vtkm::cont::ArrayHandle<T, S>& ah)
{
  return WrapArrayHandle(ah, typename IsReadOnly<vtkm::cont::ArrayHandle<T, S> >::type{});
}

template <typename T>
ArrayHandleWrapperBase<T>* MakeArrayHandleWrapper(vtkIdType numberOfTuples, int numberOfComponents)
{
  switch (numberOfComponents)
  {
    case 1:
    {
      vtkm::cont::ArrayHandle<T> ah;
      ah.Allocate(numberOfTuples);
      return MakeArrayHandleWrapper(ah);
    }
    case 2:
    {
      vtkm::cont::ArrayHandle<vtkm::Vec<T, 2> > ah;
      ah.Allocate(numberOfTuples);
      return MakeArrayHandleWrapper(ah);
    }
    case 3:
    {
      vtkm::cont::ArrayHandle<vtkm::Vec<T, 3> > ah;
      ah.Allocate(numberOfTuples);
      return MakeArrayHandleWrapper(ah);
    }
    case 4:
    {
      vtkm::cont::ArrayHandle<vtkm::Vec<T, 4> > ah;
      ah.Allocate(numberOfTuples);
      return MakeArrayHandleWrapper(ah);
    }
    default:
    {
      vtkm::cont::ArrayHandle<T> ah;
      ah.Allocate(numberOfTuples * numberOfComponents);
      return new ArrayHandleWrapperFlatSOA<T>{ ah, numberOfComponents };
    }
  }
}

} // internal

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
template <typename V, typename S>
void vtkmDataArray<T>::SetVtkmArrayHandle(const vtkm::cont::ArrayHandle<V, S>& ah)
{
  static_assert(std::is_same<T, typename internal::FlattenVec<V>::ComponentType>::value,
    "Component type of the arrays don't match");

  this->VtkmArray.reset(internal::MakeArrayHandleWrapper(ah));

  this->Size = this->VtkmArray->GetNumberOfTuples() * this->VtkmArray->GetNumberOfComponents();
  this->MaxId = this->Size - 1;
  this->SetNumberOfComponents(this->VtkmArray->GetNumberOfComponents());
}

template <typename T>
vtkm::cont::VariantArrayHandle vtkmDataArray<T>::GetVtkmVariantArrayHandle() const
{
  return this->VtkmArray->GetVtkmVariantArrayHandle();
}

template <typename T>
auto vtkmDataArray<T>::GetValue(vtkIdType valueIdx) const -> ValueType
{
  auto idx = valueIdx / this->NumberOfComponents;
  auto comp = valueIdx % this->NumberOfComponents;
  return this->VtkmArray->GetComponent(idx, comp);
}

template <typename T>
void vtkmDataArray<T>::SetValue(vtkIdType valueIdx, ValueType value)
{
  auto idx = valueIdx / this->NumberOfComponents;
  auto comp = valueIdx % this->NumberOfComponents;
  this->VtkmArray->SetComponent(idx, comp, value);
}

template <typename T>
void vtkmDataArray<T>::GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
{
  this->VtkmArray->GetTuple(tupleIdx, tuple);
}

template <typename T>
void vtkmDataArray<T>::SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
{
  this->VtkmArray->SetTuple(tupleIdx, tuple);
}

template <typename T>
auto vtkmDataArray<T>::GetTypedComponent(vtkIdType tupleIdx, int compIdx) const -> ValueType
{
  return this->VtkmArray->GetComponent(tupleIdx, compIdx);
}

template <typename T>
void vtkmDataArray<T>::SetTypedComponent(vtkIdType tupleIdx, int compIdx, ValueType value)
{
  this->VtkmArray->SetComponent(tupleIdx, compIdx, value);
}

template <typename T>
bool vtkmDataArray<T>::AllocateTuples(vtkIdType numTuples)
{
  if (this->VtkmArray && this->VtkmArray->GetNumberOfComponents() == this->NumberOfComponents)
  {
    this->VtkmArray->Allocate(numTuples);
  }
  else
  {
    this->VtkmArray.reset(internal::MakeArrayHandleWrapper<T>(numTuples, this->NumberOfComponents));
  }
  return true;
}

template <typename T>
bool vtkmDataArray<T>::ReallocateTuples(vtkIdType numTuples)
{
  this->VtkmArray->Reallocate(numTuples);
  return true;
}

#endif // vtkmDataArray_hxx
