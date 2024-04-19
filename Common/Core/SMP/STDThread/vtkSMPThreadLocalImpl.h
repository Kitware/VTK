// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkSMPThreadLocalImpl - A thread local storage implementation using
// platform specific facilities.

#ifndef STDThreadvtkSMPThreadLocalImpl_h
#define STDThreadvtkSMPThreadLocalImpl_h

#include "SMP/Common/vtkSMPThreadLocalImplAbstract.h"
#include "SMP/STDThread/vtkSMPThreadLocalBackend.h"
#include "SMP/STDThread/vtkSMPToolsImpl.txx"

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
class vtkSMPThreadLocalImpl<BackendType::STDThread, T> : public vtkSMPThreadLocalImplAbstract<T>
{
  typedef typename vtkSMPThreadLocalImplAbstract<T>::ItImpl ItImplAbstract;

public:
  vtkSMPThreadLocalImpl()
    : Backend(GetNumberOfThreadsSTDThread())
  {
  }

  explicit vtkSMPThreadLocalImpl(const T& exemplar)
    : Backend(GetNumberOfThreadsSTDThread())
    , Exemplar(exemplar)
  {
  }

  ~vtkSMPThreadLocalImpl() override
  {
    vtk::detail::smp::STDThread::ThreadSpecificStorageIterator it;
    it.SetThreadSpecificStorage(this->Backend);
    for (it.SetToBegin(); !it.GetAtEnd(); it.Forward())
    {
      delete reinterpret_cast<T*>(it.GetStorage());
    }
  }

  T& Local() override
  {
    vtk::detail::smp::STDThread::StoragePointerType& ptr = this->Backend.GetStorage();
    T* local = reinterpret_cast<T*>(ptr);
    if (!ptr)
    {
      ptr = local = new T(this->Exemplar);
    }
    return *local;
  }

  size_t size() const override { return this->Backend.GetSize(); }

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
    ItImpl* CloneImpl() const override { return new ItImpl(*this); }

  private:
    vtk::detail::smp::STDThread::ThreadSpecificStorageIterator Impl;

    friend class vtkSMPThreadLocalImpl<BackendType::STDThread, T>;
  };

  std::unique_ptr<ItImplAbstract> begin() override
  {
    // XXX(c++14): use std::make_unique
    auto it = std::unique_ptr<ItImpl>(new ItImpl());
    it->Impl.SetThreadSpecificStorage(this->Backend);
    it->Impl.SetToBegin();
    // XXX(c++14): remove std::move and cast variable
    std::unique_ptr<ItImplAbstract> abstractIt(std::move(it));
    return abstractIt;
  }

  std::unique_ptr<ItImplAbstract> end() override
  {
    // XXX(c++14): use std::make_unique
    auto it = std::unique_ptr<ItImpl>(new ItImpl());
    it->Impl.SetThreadSpecificStorage(this->Backend);
    it->Impl.SetToEnd();
    // XXX(c++14): remove std::move and cast variable
    std::unique_ptr<ItImplAbstract> abstractIt(std::move(it));
    return abstractIt;
  }

private:
  vtk::detail::smp::STDThread::ThreadSpecific Backend;
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
