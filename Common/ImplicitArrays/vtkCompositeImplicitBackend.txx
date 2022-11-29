/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeImplicitBackend.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeImplicitBackend.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchImplicitArrayList.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkSmartPointer.h"

namespace
{
//-----------------------------------------------------------------------
template <typename ValueType>
struct TypedArrayCache
{
  virtual ValueType GetValue(int idx) const = 0;
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

  ValueType GetValue(int idx) const override
  {
    return static_cast<ValueType>(this->Array->GetValue(idx));
  }

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

  ValueType GetValue(int idx) const override
  {
    int iTup = idx / this->Array->GetNumberOfComponents();
    int iComp = idx - iTup * this->Array->GetNumberOfComponents();
    return static_cast<ValueType>(this->Array->GetComponent(iTup, iComp));
  }

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

  ValueType operator()(int idx) const { return this->Cache->GetValue(idx); }

private:
  using Dispatcher = vtkArrayDispatch::DispatchByArray<ArrayList>;
  std::shared_ptr<TypedArrayCache<ValueType>> Cache = nullptr;
};
}

VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------
template <typename ValueType>
struct vtkCompositeImplicitBackend<ValueType>::Internals
{
  using InternalArrayList = vtkArrayDispatch::AllArrays;
  using CachedBackend = ::TypedCacheWrapper<InternalArrayList, ValueType>;
  using CachedArray = vtkImplicitArray<CachedBackend>;

  template <class Iterator>
  Internals(Iterator first, Iterator last)
  {
    this->CachedArrays.resize(std::distance(first, last));
    std::transform(first, last, this->CachedArrays.begin(), [](vtkDataArray* arr) {
      vtkNew<CachedArray> newCache;
      newCache->SetBackend(std::make_shared<CachedBackend>(arr));
      newCache->SetNumberOfComponents(1);
      newCache->SetNumberOfTuples(arr->GetNumberOfTuples() * arr->GetNumberOfComponents());
      return newCache;
    });
    if (this->CachedArrays.size() > 0)
    {
      this->Offsets.resize(this->CachedArrays.size() - 1);
      std::size_t runningSum = 0;
      std::transform(this->CachedArrays.begin(), this->CachedArrays.end() - 1,
        this->Offsets.begin(), [&runningSum](CachedArray* arr) {
          runningSum += arr->GetNumberOfTuples();
          return runningSum;
        });
    }
  }

  std::vector<vtkSmartPointer<CachedArray>> CachedArrays;
  std::vector<std::size_t> Offsets;
};

//-----------------------------------------------------------------------
template <typename ValueType>
vtkCompositeImplicitBackend<ValueType>::vtkCompositeImplicitBackend(
  const std::vector<vtkDataArray*>& arrays)
  : Internal(std::unique_ptr<Internals>(new Internals(arrays.begin(), arrays.end())))
{
}

//-----------------------------------------------------------------------
template <typename ValueType>
vtkCompositeImplicitBackend<ValueType>::~vtkCompositeImplicitBackend() = default;

//-----------------------------------------------------------------------
template <typename ValueType>
ValueType vtkCompositeImplicitBackend<ValueType>::operator()(int idx) const
{
  auto itPos =
    std::upper_bound(this->Internal->Offsets.begin(), this->Internal->Offsets.end(), idx);
  int locIdx = itPos == this->Internal->Offsets.begin() ? idx : idx - *(itPos - 1);
  return this->Internal->CachedArrays[std::distance(this->Internal->Offsets.begin(), itPos)]
    ->GetValue(locIdx);
}
VTK_ABI_NAMESPACE_END
