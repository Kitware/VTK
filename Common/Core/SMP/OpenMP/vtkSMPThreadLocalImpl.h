// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkSMPThreadLocalImpl - A thread local storage implementation using
// platform specific facilities.

#ifndef OpenMPvtkSMPThreadLocalImpl_h
#define OpenMPvtkSMPThreadLocalImpl_h

#include "SMP/Common/vtkSMPThreadLocalImplAbstract.h"
#include "SMP/OpenMP/vtkSMPThreadLocalBackend.h"
#include "SMP/OpenMP/vtkSMPToolsImpl.txx"

#include <iterator>
#include <utility> // For std::move

namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN

template <typename T>
class vtkSMPThreadLocalImpl<BackendType::OpenMP, T> : public vtkSMPThreadLocalImplAbstract<T>
{
  typedef typename vtkSMPThreadLocalImplAbstract<T>::ItImpl ItImplAbstract;

public:
  vtkSMPThreadLocalImpl()
    : Backend(GetNumberOfThreadsOpenMP())
  {
  }

  explicit vtkSMPThreadLocalImpl(const T& exemplar)
    : Backend(GetNumberOfThreadsOpenMP())
    , Exemplar(exemplar)
  {
  }

  ~vtkSMPThreadLocalImpl() override
  {
    OpenMP::ThreadSpecificStorageIterator it;
    it.SetThreadSpecificStorage(Backend);
    for (it.SetToBegin(); !it.GetAtEnd(); it.Forward())
    {
      delete reinterpret_cast<T*>(it.GetStorage());
    }
  }

  T& Local() override
  {
    OpenMP::StoragePointerType& ptr = this->Backend.GetStorage();
    T* local = reinterpret_cast<T*>(ptr);
    if (!ptr)
    {
      ptr = local = new T(this->Exemplar);
    }
    return *local;
  }

  size_t size() const override { return this->Backend.Size(); }

  class ItImpl : public vtkSMPThreadLocalImplAbstract<T>::ItImpl
  {
  public:
    void Increment() override { this->Impl.Forward(); }

    bool Compare(ItImplAbstract* other) override
    {
      return this->Impl == static_cast<ItImpl*>(other)->Impl;
    }

    T& GetContent() override { return *reinterpret_cast<T*>(this->Impl.GetStorage()); }

    T* GetContentPtr() override { return reinterpret_cast<T*>(this->Impl.GetStorage()); }

  protected:
    ItImpl* CloneImpl() const override { return new ItImpl(*this); };

  private:
    OpenMP::ThreadSpecificStorageIterator Impl;

    friend class vtkSMPThreadLocalImpl<BackendType::OpenMP, T>;
  };

  std::unique_ptr<ItImplAbstract> begin() override
  {
    auto it = std::make_unique<ItImpl>();
    it->Impl.SetThreadSpecificStorage(this->Backend);
    it->Impl.SetToBegin();
    return it;
  }

  std::unique_ptr<ItImplAbstract> end() override
  {
    auto it = std::make_unique<ItImpl>();
    it->Impl.SetThreadSpecificStorage(this->Backend);
    it->Impl.SetToEnd();
    return it;
  }

private:
  OpenMP::ThreadSpecific Backend;
  T Exemplar;

  // disable copying
  vtkSMPThreadLocalImpl(const vtkSMPThreadLocalImpl&) = delete;
  void operator=(const vtkSMPThreadLocalImpl&) = delete;
};

VTK_ABI_NAMESPACE_END
} // namespace smp
} // namespace detail
} // namespace vtk

#endif
/* VTK-HeaderTest-Exclude: vtkSMPThreadLocalImpl.h */
