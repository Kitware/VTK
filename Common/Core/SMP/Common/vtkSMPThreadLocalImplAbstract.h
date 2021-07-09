/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkSMPThreadLocalImplAbstract.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
    ItImpl(ItImpl&&) = default;
    ItImpl& operator=(const ItImpl&) = default;
    ItImpl& operator=(ItImpl&&) = default;

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

} // namespace smp
} // namespace detail
} // namespace vtk

#endif
