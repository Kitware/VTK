// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIndexedImplicitBackend.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkIdList.h"
#include "vtkImplicitArray.h"
#include "vtkTypeList.h"

#include <memory>

namespace vtkIndexedImplicitBackendDetail
{
VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------
template <typename ValueType>
struct TypedArrayCache
{
  virtual ValueType GetValue(vtkIdType idx) const = 0;
  virtual unsigned long getMemorySize() const = 0;
  virtual ~TypedArrayCache() = default;
};

template <typename ValueType, typename ArrayT>
struct SpecializedCache : public TypedArrayCache<ValueType>
{
public:
  SpecializedCache(ArrayT* arr)
    : Array(arr)
  {
  }

  ValueType GetValue(vtkIdType idx) const override
  {
    return static_cast<ValueType>(this->Array->GetValue(idx));
  }

  unsigned long getMemorySize() const override { return this->Array->GetActualMemorySize(); };

private:
  vtkSmartPointer<ArrayT> Array;
};

template <typename ValueType>
struct SpecializedCache<ValueType, vtkDataArray> : public TypedArrayCache<ValueType>
{
public:
  SpecializedCache(vtkDataArray* arr)
    : Array(arr)
  {
  }

  ValueType GetValue(vtkIdType idx) const override
  {
    const int nComps = this->Array->GetNumberOfComponents();
    const vtkIdType iTup = idx / nComps;
    const int iComp = idx - iTup * nComps;
    return static_cast<ValueType>(this->Array->GetComponent(iTup, iComp));
  }

  unsigned long getMemorySize() const override { return this->Array->GetActualMemorySize(); };

private:
  vtkSmartPointer<vtkDataArray> Array;
};

//-----------------------------------------------------------------------
template <typename ValueType>
struct CacheDispatchWorker
{
  template <typename ArrayT>
  void operator()(ArrayT* arr, std::shared_ptr<TypedArrayCache<ValueType>>& cache)
  {
    cache = std::make_shared<SpecializedCache<ValueType, ArrayT>>(arr);
  }
};

//-----------------------------------------------------------------------
template <typename ArrayList, typename ValueType>
struct TypedCacheWrapper
{
  TypedCacheWrapper(vtkDataArray* arr)
  {
    CacheDispatchWorker<ValueType> worker;
    if (!Dispatcher::Execute(arr, worker, this->Cache))
    {
      worker(arr, this->Cache);
    }
  }

  ValueType operator()(vtkIdType idx) const { return this->Cache->GetValue(idx); }

  unsigned long getMemorySize() const { return this->Cache->getMemorySize(); }

private:
  using Dispatcher = vtkArrayDispatch::DispatchByArray<ArrayList>;
  std::shared_ptr<TypedArrayCache<ValueType>> Cache = nullptr;
};

//-----------------------------------------------------------------------
struct IdListWrapper
{
  IdListWrapper(vtkIdList* indexes)
    : Handles(indexes)
  {
  }

  vtkIdType operator()(vtkIdType idx) const { return this->Handles->GetId(idx); }

  unsigned long getMemorySize() const
  {
    size_t bytes = sizeof(vtkIdType) * this->Handles->GetNumberOfIds();
    return std::ceil(bytes / 1024.0);
  }

  vtkSmartPointer<vtkIdList> Handles;
};
VTK_ABI_NAMESPACE_END
} // namespace vtkIndexedImplicitBackendDetail

VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------
template <typename ValueType>
struct vtkIndexedImplicitBackend<ValueType>::Internals
{
  using InternalArrayList = vtkTypeList::Append<vtkArrayDispatch::AllArrays,
    vtkTypeList::Create<vtkImplicitArray<vtkIndexedImplicitBackendDetail::IdListWrapper>>>::Result;

  Internals(vtkIdList* indexes, vtkDataArray* array)
  {
    if (!indexes || !array)
    {
      vtkErrorWithObjectMacro(nullptr, "Either index array or array itself is nullptr");
      return;
    }
    vtkNew<vtkImplicitArray<vtkIndexedImplicitBackendDetail::IdListWrapper>> newHandles;
    newHandles->SetBackend(
      std::make_shared<vtkIndexedImplicitBackendDetail::IdListWrapper>(indexes));
    newHandles->SetNumberOfComponents(1);
    newHandles->SetNumberOfTuples(indexes->GetNumberOfIds());
    this->Handles = this->TypeCacheArray<vtkIdType>(newHandles);
    this->Array = this->TypeCacheArray<ValueType>(array);
  }

  Internals(vtkDataArray* indexes, vtkDataArray* array)
  {
    if (!indexes || !array)
    {
      vtkErrorWithObjectMacro(nullptr, "Either index array or array itself is nullptr");
      return;
    }
    if (indexes->GetNumberOfComponents() != 1)
    {
      vtkErrorWithObjectMacro(nullptr,
        "Passed a vtkDataArray with multiple components as indexing array to vtkIndexedArray");
      return;
    }
    this->Handles = this->TypeCacheArray<vtkIdType>(indexes);
    this->Array = this->TypeCacheArray<ValueType>(array);
  }

  template <typename VT>
  static vtkSmartPointer<
    vtkImplicitArray<vtkIndexedImplicitBackendDetail::TypedCacheWrapper<InternalArrayList, VT>>>
  TypeCacheArray(vtkDataArray* da)
  {
    vtkNew<
      vtkImplicitArray<vtkIndexedImplicitBackendDetail::TypedCacheWrapper<InternalArrayList, VT>>>
      wrapped;
    wrapped->SetBackend(
      std::make_shared<vtkIndexedImplicitBackendDetail::TypedCacheWrapper<InternalArrayList, VT>>(
        da));
    wrapped->SetNumberOfComponents(1);
    wrapped->SetNumberOfTuples(da->GetNumberOfTuples() * da->GetNumberOfComponents());
    return wrapped;
  }

  vtkSmartPointer<vtkImplicitArray<
    vtkIndexedImplicitBackendDetail::TypedCacheWrapper<InternalArrayList, ValueType>>>
    Array;
  vtkSmartPointer<vtkImplicitArray<
    vtkIndexedImplicitBackendDetail::TypedCacheWrapper<InternalArrayList, vtkIdType>>>
    Handles;
};

//-----------------------------------------------------------------------
template <typename ValueType>
vtkIndexedImplicitBackend<ValueType>::vtkIndexedImplicitBackend(
  vtkIdList* indexes, vtkDataArray* array)
  : Internal(std::unique_ptr<Internals>(new Internals(indexes, array)))
{
}

//-----------------------------------------------------------------------
template <typename ValueType>
vtkIndexedImplicitBackend<ValueType>::vtkIndexedImplicitBackend(
  vtkDataArray* indexes, vtkDataArray* array)
  : Internal(std::unique_ptr<Internals>(new Internals(indexes, array)))
{
}

//-----------------------------------------------------------------------
template <typename ValueType>
vtkIndexedImplicitBackend<ValueType>::~vtkIndexedImplicitBackend() = default;

//-----------------------------------------------------------------------
template <typename ValueType>
ValueType vtkIndexedImplicitBackend<ValueType>::operator()(vtkIdType idx) const
{
  return this->Internal->Array->GetValue(this->Internal->Handles->GetValue(idx));
}

template <typename ValueType>
unsigned long vtkIndexedImplicitBackend<ValueType>::getMemorySize() const
{
  return this->Internal->Array->GetActualMemorySize() +
    this->Internal->Handles->GetActualMemorySize();
}
VTK_ABI_NAMESPACE_END
