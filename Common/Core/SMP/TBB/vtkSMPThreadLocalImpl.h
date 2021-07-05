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
// .NAME vtkSMPThreadLocal - A TBB based thread local storage implementation.

#ifndef TBBvtkSMPThreadLocalImpl_h
#define TBBvtkSMPThreadLocalImpl_h

#include "SMP/Common/vtkSMPThreadLocalImplAbstract.h"

#ifdef _MSC_VER
#pragma push_macro("__TBB_NO_IMPLICIT_LINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1
#endif

#include <tbb/enumerable_thread_specific.h>

#ifdef _MSC_VER
#pragma pop_macro("__TBB_NO_IMPLICIT_LINKAGE")
#endif

#include <iterator>
#include <utility> // For std::move

namespace vtk
{
namespace detail
{
namespace smp
{

template <typename T>
class vtkSMPThreadLocalImpl<BackendType::TBB, T> : public vtkSMPThreadLocalImplAbstract<T>
{
  typedef tbb::enumerable_thread_specific<T> TLS;
  typedef typename TLS::iterator TLSIter;
  typedef typename vtkSMPThreadLocalImplAbstract<T>::ItImpl ItImplAbstract;

public:
  vtkSMPThreadLocalImpl() {}

  explicit vtkSMPThreadLocalImpl(const T& exemplar)
    : Internal(exemplar)
  {
  }

  T& Local() override { return this->Internal.local(); }

  size_t size() const override { return this->Internal.size(); }

  class ItImpl : public vtkSMPThreadLocalImplAbstract<T>::ItImpl
  {
  public:
    void Increment() override { ++this->Iter; }

    bool Compare(ItImplAbstract* other) override
    {
      return this->Iter == static_cast<ItImpl*>(other)->Iter;
    }

    T& GetContent() override { return *this->Iter; }

    T* GetContentPtr() override { return &*this->Iter; }

  protected:
    virtual ItImpl* CloneImpl() const override { return new ItImpl(*this); };

  private:
    TLSIter Iter;

    friend class vtkSMPThreadLocalImpl<BackendType::TBB, T>;
  };

  std::unique_ptr<ItImplAbstract> begin() override
  {
    // XXX(c++14): use std::make_unique
    auto iter = std::unique_ptr<ItImpl>(new ItImpl());
    iter->Iter = this->Internal.begin();
    // XXX(c++14): remove std::move and cast variable
    std::unique_ptr<ItImplAbstract> abstractIt(std::move(iter));
    return abstractIt;
  };

  std::unique_ptr<ItImplAbstract> end() override
  {
    // XXX(c++14): use std::make_unique
    auto iter = std::unique_ptr<ItImpl>(new ItImpl());
    iter->Iter = this->Internal.end();
    // XXX(c++14): remove std::move and cast variable
    std::unique_ptr<ItImplAbstract> abstractIt(std::move(iter));
    return abstractIt;
  }

private:
  TLS Internal;

  // disable copying
  vtkSMPThreadLocalImpl(const vtkSMPThreadLocalImpl&) = delete;
  void operator=(const vtkSMPThreadLocalImpl&) = delete;
};

} // namespace smp
} // namespace detail
} // namespace vtk

#endif
