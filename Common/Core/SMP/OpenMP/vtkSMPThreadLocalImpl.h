/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPThreadLocalImpl.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
    virtual ItImpl* CloneImpl() const override { return new ItImpl(*this); };

  private:
    OpenMP::ThreadSpecificStorageIterator Impl;

    friend class vtkSMPThreadLocalImpl<BackendType::OpenMP, T>;
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
  OpenMP::ThreadSpecific Backend;
  T Exemplar;

  // disable copying
  vtkSMPThreadLocalImpl(const vtkSMPThreadLocalImpl&) = delete;
  void operator=(const vtkSMPThreadLocalImpl&) = delete;
};

} // namespace smp
} // namespace detail
} // namespace vtk

#endif
