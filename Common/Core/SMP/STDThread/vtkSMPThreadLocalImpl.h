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
// .SECTION Description
// A thread local object is one that maintains a copy of an object of the
// template type for each thread that processes data. vtkSMPThreadLocal
// creates storage for all threads but the actual objects are created
// the first time Local() is called. Note that some of the vtkSMPThreadLocal
// API is not thread safe. It can be safely used in a multi-threaded
// environment because Local() returns storage specific to a particular
// thread, which by default will be accessed sequentially. It is also
// thread-safe to iterate over vtkSMPThreadLocal as long as each thread
// creates its own iterator and does not change any of the thread local
// objects.
//
// A common design pattern in using a thread local storage object is to
// write/accumulate data to local object when executing in parallel and
// then having a sequential code block that iterates over the whole storage
// using the iterators to do the final accumulation.

#ifndef STDThreadvtkSMPThreadLocalImpl_h
#define STDThreadvtkSMPThreadLocalImpl_h

#include "SMP/Common/vtkSMPThreadLocalImplAbstract.h"
#include "SMP/STDThread/vtkSMPThreadLocalBackend.h"
#include "SMP/STDThread/vtkSMPToolsImpl.txx"

#include <iterator>

namespace vtk
{
namespace detail
{
namespace smp
{

template <typename T>
class vtkSMPThreadLocalImpl<BackendType::STDThread, T> : public vtkSMPThreadLocalImplAbstract<T>
{
  typedef typename vtkSMPThreadLocalImplAbstract<T>::ItImpl ItImplAbstract;

public:
  // Description:
  // Default constructor. Creates a default exemplar.
  vtkSMPThreadLocalImpl()
    : Backend(GetNumberOfThreadsSTDThread())
  {
  }

  // Description:
  // Constructor that allows the specification of an exemplar object
  // which is used when constructing objects when Local() is first called.
  // Note that a copy of the exemplar is created using its copy constructor.
  explicit vtkSMPThreadLocalImpl(const T& exemplar)
    : Backend(GetNumberOfThreadsSTDThread())
    , Exemplar(exemplar)
  {
  }

  ~vtkSMPThreadLocalImpl()
  {
    vtk::detail::smp::STDThread::ThreadSpecificStorageIterator it;
    it.SetThreadSpecificStorage(this->Backend);
    for (it.SetToBegin(); !it.GetAtEnd(); it.Forward())
    {
      delete reinterpret_cast<T*>(it.GetStorage());
    }
  }

  // Description:
  // Returns an object of type T that is local to the current thread.
  // This needs to be called mainly within a threaded execution path.
  // It will create a new object (local to the thread so each thread
  // get their own when calling Local) which is a copy of exemplar as passed
  // to the constructor (or a default object if no exemplar was provided)
  // the first time it is called. After the first time, it will return
  // the same object.
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

  // Description:
  // Return the number of thread local objects that have been allocated
  size_t size() const override { return this->Backend.GetSize(); }

  // Description:
  // Subset of the standard iterator API.
  // The most common design pattern is to use iterators in a sequential
  // code block and to use only the thread local objects in parallel
  // code blocks.
  // It is thread safe to iterate over the thread local containers
  // as long as each thread uses its own iterator and does not modify
  // objects in the container.
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
    vtk::detail::smp::STDThread::ThreadSpecificStorageIterator Impl;

    friend class vtkSMPThreadLocalImpl<BackendType::STDThread, T>;
  };

  // Description:
  // Returns a new iterator pointing to the beginning of
  // the local storage container. Thread safe.
  std::unique_ptr<ItImplAbstract> begin() override
  {
    // XXX(c++14): use std::make_unique
    auto it = std::unique_ptr<ItImpl>(new ItImpl());
    it->Impl.SetThreadSpecificStorage(this->Backend);
    it->Impl.SetToBegin();
    return it;
  }

  // Description:
  // Returns a new iterator pointing to past the end of
  // the local storage container. Thread safe.
  std::unique_ptr<ItImplAbstract> end() override
  {
    // XXX(c++14): use std::make_unique
    auto it = std::unique_ptr<ItImpl>(new ItImpl());
    it->Impl.SetThreadSpecificStorage(this->Backend);
    it->Impl.SetToEnd();
    return it;
  }

private:
  vtk::detail::smp::STDThread::ThreadSpecific Backend;
  T Exemplar;

  // disable copying
  vtkSMPThreadLocalImpl(const vtkSMPThreadLocalImpl&) = delete;
  void operator=(const vtkSMPThreadLocalImpl&) = delete;
};

} // namespace smp
} // namespace detail
} // namespace vtk

#endif
