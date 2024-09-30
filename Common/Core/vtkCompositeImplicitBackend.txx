// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCompositeImplicitBackend.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkArrayDispatch.h"
#include "vtkCollectionRange.h"
#include "vtkDataArray.h"
#include "vtkDataArrayCollection.h"
#include "vtkImplicitArray.h"
#include "vtkSmartPointer.h"

namespace vtkCompositeImplicitBackendDetail
{
VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------
/*
 * A generic interface towards a typed get value. Specialized structures should inherit from this
 * struct in order to abstractify the array type from the GetValue usage.
 */
template <typename ValueType>
struct TypedArrayCache
{
  virtual ValueType GetValue(vtkIdType idx) const = 0;
  virtual ~TypedArrayCache() = default;
  virtual unsigned long getMemorySize() const = 0;
};

/*
 * A templated implementation of the TypedArrayCache above that should be used for arrays that
 * implement the `GetValue` method from the `vtkGenericDataArray` interface.
 */
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

/*
 * An implementation of TypedArrayCache for `vtkDataArray` that acts as a fallback implementation
 * for arrays whose base type cannot be determined by `vtkArrayDispatch`.
 */
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
    vtkIdType iTup = idx / this->Array->GetNumberOfComponents();
    int iComp = idx - iTup * this->Array->GetNumberOfComponents();
    return static_cast<ValueType>(this->Array->GetComponent(iTup, iComp));
  }

  unsigned long getMemorySize() const override { return this->Array->GetActualMemorySize(); };

private:
  vtkSmartPointer<vtkDataArray> Array;
};

//-----------------------------------------------------------------------
/*
 * A worker structure to be used with `vtkArrayDispatch` in order to cache typed versions of arrays
 * once for improving random access speed.
 */
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
/*
 * A structure that wraps around a TypedArrayCache and can serve as a backend to a
 * `vtkImplicitArray`. Its constructor is what uses dispatches the underlying array into a typed
 * cache.
 */
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
VTK_ABI_NAMESPACE_END
} // namespace vtkCompositeImplicitBackendDetail

VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------
template <typename ValueType>
struct vtkCompositeImplicitBackend<ValueType>::Internals
{
  using InternalArrayList = vtkArrayDispatch::AllArrays;
  using CachedBackend =
    vtkCompositeImplicitBackendDetail::TypedCacheWrapper<InternalArrayList, ValueType>;
  using CachedArray = vtkImplicitArray<CachedBackend>;

  /*
   * Construct an internal structure from any range of iterators providing a stream of
   * `vtkDataArray*`s. At construction, every array is dispatched into a cache and the offsets are
   * calculated to enable fast binary search in `vtkCompositeImplicitBackend::operator()`.
   */
  template <class Iterator>
  Internals(Iterator first, Iterator last)
  {
    this->Initialize(first, last);
  }

  Internals(vtkDataArrayCollection* collection)
  {
    auto arrayRange = vtk::Range(collection);
    this->Initialize(arrayRange.begin(), arrayRange.end());
  }

  std::vector<vtkSmartPointer<CachedArray>> CachedArrays;
  std::vector<vtkIdType> Offsets;

private:
  template <class Iterator>
  void Initialize(Iterator first, Iterator last)
  {
    this->CachedArrays.resize(std::distance(first, last));
    std::transform(first, last, this->CachedArrays.begin(),
      [](vtkDataArray* arr)
      {
        vtkNew<CachedArray> newCache;
        newCache->SetBackend(std::make_shared<CachedBackend>(arr));
        newCache->SetNumberOfComponents(1);
        if (arr)
        {
          newCache->SetNumberOfTuples(arr->GetNumberOfTuples() * arr->GetNumberOfComponents());
        }
        else
        {
          newCache->SetNumberOfTuples(0);
        }
        return newCache;
      });
    if (this->CachedArrays.size() > 0)
    {
      this->Offsets.resize(this->CachedArrays.size() - 1);
      std::size_t runningSum = 0;
      std::transform(this->CachedArrays.begin(), this->CachedArrays.end() - 1,
        this->Offsets.begin(),
        [&runningSum](CachedArray* arr)
        {
          runningSum += arr->GetNumberOfTuples();
          return runningSum;
        });
    }
  }
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
vtkCompositeImplicitBackend<ValueType>::vtkCompositeImplicitBackend(vtkDataArrayCollection* arrays)
  : Internal(std::unique_ptr<Internals>(new Internals(arrays)))
{
}

//-----------------------------------------------------------------------
template <typename ValueType>
vtkCompositeImplicitBackend<ValueType>::~vtkCompositeImplicitBackend() = default;

//-----------------------------------------------------------------------
template <typename ValueType>
ValueType vtkCompositeImplicitBackend<ValueType>::operator()(vtkIdType idx) const
{
  auto itPos =
    std::upper_bound(this->Internal->Offsets.begin(), this->Internal->Offsets.end(), idx);
  vtkIdType locIdx = itPos == this->Internal->Offsets.begin() ? idx : idx - *(itPos - 1);
  return this->Internal->CachedArrays[std::distance(this->Internal->Offsets.begin(), itPos)]
    ->GetValue(locIdx);
}

template <typename ValueType>
unsigned long vtkCompositeImplicitBackend<ValueType>::getMemorySize() const
{
  unsigned long arraySizeSum = 0;
  for (const auto& array : this->Internal->CachedArrays)
  {
    arraySizeSum += array->GetActualMemorySize();
  }

  return arraySizeSum;
}

VTK_ABI_NAMESPACE_END
