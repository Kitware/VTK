// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMPThreadLocalAPI_h
#define vtkSMPThreadLocalAPI_h

#include <array>
#include <iterator>
#include <memory>

#include "SMP/Common/vtkSMPThreadLocalImplAbstract.h"
#include "SMP/Common/vtkSMPToolsAPI.h" // For GetBackendType(), DefaultBackend
#include "vtkSMP.h"
#include "vtkSystemIncludes.h"

#if VTK_SMP_ENABLE_SEQUENTIAL
#include "SMP/Sequential/vtkSMPThreadLocalImpl.h"
#endif
#if VTK_SMP_ENABLE_STDTHREAD
#include "SMP/STDThread/vtkSMPThreadLocalImpl.h"
#endif
#if VTK_SMP_ENABLE_TBB
#include "SMP/TBB/vtkSMPThreadLocalImpl.h"
#endif
#if VTK_SMP_ENABLE_OPENMP
#include "SMP/OpenMP/vtkSMPThreadLocalImpl.h"
#endif

namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN

template <typename T>
class vtkSMPThreadLocalAPI
{
#if VTK_SMP_ENABLE_SEQUENTIAL
  using ThreadLocalSequential = vtkSMPThreadLocalImpl<BackendType::Sequential, T>;
#endif
#if VTK_SMP_ENABLE_STDTHREAD
  using ThreadLocalSTDThread = vtkSMPThreadLocalImpl<BackendType::STDThread, T>;
#endif
#if VTK_SMP_ENABLE_TBB
  using ThreadLocalTBB = vtkSMPThreadLocalImpl<BackendType::TBB, T>;
#endif
#if VTK_SMP_ENABLE_OPENMP
  using ThreadLocalOpenMP = vtkSMPThreadLocalImpl<BackendType::OpenMP, T>;
#endif
  typedef typename vtkSMPThreadLocalImplAbstract<T>::ItImpl ItImplAbstract;

public:
  //--------------------------------------------------------------------------------
  vtkSMPThreadLocalAPI()
  {
#if VTK_SMP_ENABLE_SEQUENTIAL
    this->BackendsImpl[static_cast<int>(BackendType::Sequential)] =
      std::make_unique<ThreadLocalSequential>();
#endif
#if VTK_SMP_ENABLE_STDTHREAD
    this->BackendsImpl[static_cast<int>(BackendType::STDThread)] =
      std::make_unique<ThreadLocalSTDThread>();
#endif
#if VTK_SMP_ENABLE_TBB
    this->BackendsImpl[static_cast<int>(BackendType::TBB)] = std::make_unique<ThreadLocalTBB>();
#endif
#if VTK_SMP_ENABLE_OPENMP
    this->BackendsImpl[static_cast<int>(BackendType::OpenMP)] =
      std::make_unique<ThreadLocalOpenMP>();
#endif
  }

  //--------------------------------------------------------------------------------
  explicit vtkSMPThreadLocalAPI(const T& exemplar)
  {
#if VTK_SMP_ENABLE_SEQUENTIAL
    this->BackendsImpl[static_cast<int>(BackendType::Sequential)] =
      std::make_unique<ThreadLocalSequential>(exemplar);
#endif
#if VTK_SMP_ENABLE_STDTHREAD
    this->BackendsImpl[static_cast<int>(BackendType::STDThread)] =
      std::make_unique<ThreadLocalSTDThread>(exemplar);
#endif
#if VTK_SMP_ENABLE_TBB
    this->BackendsImpl[static_cast<int>(BackendType::TBB)] =
      std::make_unique<ThreadLocalTBB>(exemplar);
#endif
#if VTK_SMP_ENABLE_OPENMP
    this->BackendsImpl[static_cast<int>(BackendType::OpenMP)] =
      std::make_unique<ThreadLocalOpenMP>(exemplar);
#endif
  }

  //--------------------------------------------------------------------------------
  T& Local()
  {
    BackendType backendType = this->GetSMPBackendType();
    return this->BackendsImpl[static_cast<int>(backendType)]->Local();
  }

  //--------------------------------------------------------------------------------
  size_t size()
  {
    BackendType backendType = this->GetSMPBackendType();
    return this->BackendsImpl[static_cast<int>(backendType)]->size();
  }

  //--------------------------------------------------------------------------------
  class iterator
  {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    iterator() = default;

    iterator(const iterator& other)
      : ImplAbstract(other.ImplAbstract->Clone())
    {
    }

    iterator& operator=(const iterator& other)
    {
      if (this != &other)
      {
        this->ImplAbstract = other.ImplAbstract->Clone();
      }
      return *this;
    }

    iterator& operator++()
    {
      this->ImplAbstract->Increment();
      return *this;
    }

    iterator operator++(int)
    {
      iterator copy = *this;
      this->ImplAbstract->Increment();
      return copy;
    }

    bool operator==(const iterator& other) const
    {
      return this->ImplAbstract->Compare(other.ImplAbstract.get());
    }

    bool operator!=(const iterator& other) const
    {
      return !this->ImplAbstract->Compare(other.ImplAbstract.get());
    }

    T& operator*() { return this->ImplAbstract->GetContent(); }

    T* operator->() { return this->ImplAbstract->GetContentPtr(); }

  private:
    std::unique_ptr<ItImplAbstract> ImplAbstract;

    friend class vtkSMPThreadLocalAPI<T>;
  };

  //--------------------------------------------------------------------------------
  iterator begin()
  {
    BackendType backendType = this->GetSMPBackendType();
    iterator iter;
    iter.ImplAbstract = this->BackendsImpl[static_cast<int>(backendType)]->begin();
    return iter;
  }

  //--------------------------------------------------------------------------------
  iterator end()
  {
    BackendType backendType = this->GetSMPBackendType();
    iterator iter;
    iter.ImplAbstract = this->BackendsImpl[static_cast<int>(backendType)]->end();
    return iter;
  }

  // disable copying
  vtkSMPThreadLocalAPI(const vtkSMPThreadLocalAPI&) = delete;
  vtkSMPThreadLocalAPI& operator=(const vtkSMPThreadLocalAPI&) = delete;

private:
  std::array<std::unique_ptr<vtkSMPThreadLocalImplAbstract<T>>, VTK_SMP_MAX_BACKENDS_NB>
    BackendsImpl;

  //--------------------------------------------------------------------------------
  BackendType GetSMPBackendType()
  {
    auto& SMPToolsAPI = vtkSMPToolsAPI::GetInstance();
    return SMPToolsAPI.GetBackendType();
  }
};

VTK_ABI_NAMESPACE_END
} // namespace smp
} // namespace detail
} // namespace vtk

#endif
/* VTK-HeaderTest-Exclude: vtkSMPThreadLocalAPI.h */
