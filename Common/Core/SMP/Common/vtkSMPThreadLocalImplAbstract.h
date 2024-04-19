// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMPThreadLocalImplAbstract_h
#define vtkSMPThreadLocalImplAbstract_h

#include <memory>

#include "SMP/Common/vtkSMPToolsImpl.h"

namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN

template <typename T>
class vtkSMPThreadLocalImplAbstract
{
public:
  virtual ~vtkSMPThreadLocalImplAbstract() = default;

  virtual T& Local() = 0;

  virtual size_t size() const = 0;

  class ItImpl
  {
  public:
    ItImpl() = default;
    virtual ~ItImpl() = default;
    ItImpl(const ItImpl&) = default;
    ItImpl(ItImpl&&) noexcept = default;
    ItImpl& operator=(const ItImpl&) = default;
    ItImpl& operator=(ItImpl&&) noexcept = default;

    virtual void Increment() = 0;

    virtual bool Compare(ItImpl* other) = 0;

    virtual T& GetContent() = 0;

    virtual T* GetContentPtr() = 0;

    std::unique_ptr<ItImpl> Clone() const { return std::unique_ptr<ItImpl>(CloneImpl()); }

  protected:
    virtual ItImpl* CloneImpl() const = 0;
  };

  virtual std::unique_ptr<ItImpl> begin() = 0;

  virtual std::unique_ptr<ItImpl> end() = 0;
};

template <BackendType Backend, typename T>
class vtkSMPThreadLocalImpl : public vtkSMPThreadLocalImplAbstract<T>
{
};

VTK_ABI_NAMESPACE_END
} // namespace smp
} // namespace detail
} // namespace vtk

#endif
/* VTK-HeaderTest-Exclude: vtkSMPThreadLocalImplAbstract.h */
