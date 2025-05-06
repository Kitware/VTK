// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkSMPThreadLocal - A simple thread local implementation for sequential operations.
// .SECTION Description
//
// Note that this particular implementation is designed to work in sequential
// mode and supports only 1 thread.

#ifndef SequentialvtkSMPThreadLocalImpl_h
#define SequentialvtkSMPThreadLocalImpl_h

#include "SMP/Common/vtkSMPThreadLocalImplAbstract.h"
#include "vtkSystemIncludes.h"

#include <iterator>
#include <utility> // For std::move
#include <vector>

namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN

template <typename T>
class vtkSMPThreadLocalImpl<BackendType::Sequential, T> : public vtkSMPThreadLocalImplAbstract<T>
{
  typedef std::vector<T> TLS;
  typedef typename TLS::iterator TLSIter;
  typedef typename vtkSMPThreadLocalImplAbstract<T>::ItImpl ItImplAbstract;

public:
  vtkSMPThreadLocalImpl()
    : NumInitialized(0)
  {
    this->Initialize();
  }

  explicit vtkSMPThreadLocalImpl(const T& exemplar)
    : NumInitialized(0)
    , Exemplar(exemplar)
  {
    this->Initialize();
  }

  T& Local() override
  {
    int tid = this->GetThreadID();
    if (!this->Initialized[tid])
    {
      this->Internal[tid] = this->Exemplar;
      this->Initialized[tid] = true;
      ++this->NumInitialized;
    }
    return this->Internal[tid];
  }

  size_t size() const override { return this->NumInitialized; }

  class ItImpl : public vtkSMPThreadLocalImplAbstract<T>::ItImpl
  {
  public:
    void Increment() override
    {
      this->InitIter++;
      this->Iter++;

      // Make sure to skip uninitialized
      // entries.
      while (this->InitIter != this->EndIter)
      {
        if (*this->InitIter)
        {
          break;
        }
        this->InitIter++;
        this->Iter++;
      }
    }

    bool Compare(ItImplAbstract* other) override
    {
      return this->Iter == static_cast<ItImpl*>(other)->Iter;
    }

    T& GetContent() override { return *this->Iter; }

    T* GetContentPtr() override { return &*this->Iter; }

  protected:
    ItImpl* CloneImpl() const override { return new ItImpl(*this); }

  private:
    friend class vtkSMPThreadLocalImpl<BackendType::Sequential, T>;
    std::vector<bool>::iterator InitIter;
    std::vector<bool>::iterator EndIter;
    TLSIter Iter;
  };

  std::unique_ptr<ItImplAbstract> begin() override
  {
    TLSIter iter = this->Internal.begin();
    std::vector<bool>::iterator iter2 = this->Initialized.begin();
    std::vector<bool>::iterator enditer = this->Initialized.end();
    // fast forward to first initialized
    // value
    while (iter2 != enditer)
    {
      if (*iter2)
      {
        break;
      }
      iter2++;
      iter++;
    }
    auto retVal = std::make_unique<ItImpl>();
    retVal->InitIter = iter2;
    retVal->EndIter = enditer;
    retVal->Iter = iter;
    return retVal;
  }

  std::unique_ptr<ItImplAbstract> end() override
  {
    auto retVal = std::make_unique<ItImpl>();
    retVal->InitIter = this->Initialized.end();
    retVal->EndIter = this->Initialized.end();
    retVal->Iter = this->Internal.end();
    return retVal;
  }

private:
  TLS Internal;
  std::vector<bool> Initialized;
  size_t NumInitialized;
  T Exemplar;

  void Initialize()
  {
    this->Internal.resize(this->GetNumberOfThreads());
    this->Initialized.resize(this->GetNumberOfThreads());
    std::fill(this->Initialized.begin(), this->Initialized.end(), false);
  }

  int GetNumberOfThreads() { return 1; }

  int GetThreadID() { return 0; }

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
