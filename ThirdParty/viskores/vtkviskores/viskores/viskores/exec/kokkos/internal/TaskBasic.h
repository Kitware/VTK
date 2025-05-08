//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_exec_kokkos_internal_TaskBasic_h
#define viskores_exec_kokkos_internal_TaskBasic_h

#include <viskores/exec/TaskBase.h>

//Todo: rename this header to TaskInvokeWorkletDetail.h
#include <viskores/exec/internal/WorkletInvokeFunctorDetail.h>

namespace viskores
{
namespace exec
{
namespace kokkos
{
namespace internal
{

template <typename WType, typename IType, typename Hints>
class TaskBasic1D : public viskores::exec::TaskBase
{
  VISKORES_IS_HINT_LIST(Hints);

public:
  TaskBasic1D(const WType& worklet, const IType& invocation)
    : Worklet(worklet)
    , Invocation(invocation)
  {
  }

  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }

  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    viskores::exec::internal::detail::DoWorkletInvokeFunctor(
      this->Worklet,
      this->Invocation,
      this->Worklet.GetThreadIndices(index,
                                     this->Invocation.OutputToInputMap,
                                     this->Invocation.VisitArray,
                                     this->Invocation.ThreadToOutputMap,
                                     this->Invocation.GetInputDomain()));
  }

private:
  typename std::remove_const<WType>::type Worklet;
  IType Invocation;
};

template <typename WType, typename Hints>
class TaskBasic1D<WType, viskores::internal::NullType, Hints> : public viskores::exec::TaskBase
{
  VISKORES_IS_HINT_LIST(Hints);

public:
  explicit TaskBasic1D(const WType& worklet)
    : Worklet(worklet)
  {
  }

  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }

  VISKORES_EXEC
  void operator()(viskores::Id index) const { this->Worklet(index); }

private:
  typename std::remove_const<WType>::type Worklet;
};

template <typename WType, typename IType, typename Hints>
class TaskBasic3D : public viskores::exec::TaskBase
{
  VISKORES_IS_HINT_LIST(Hints);

public:
  TaskBasic3D(const WType& worklet, const IType& invocation)
    : Worklet(worklet)
    , Invocation(invocation)
  {
  }

  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }

  VISKORES_EXEC
  void operator()(viskores::Id3 idx, viskores::Id flatIdx) const
  {
    viskores::exec::internal::detail::DoWorkletInvokeFunctor(
      this->Worklet,
      this->Invocation,
      this->Worklet.GetThreadIndices(flatIdx,
                                     idx,
                                     this->Invocation.OutputToInputMap,
                                     this->Invocation.VisitArray,
                                     this->Invocation.ThreadToOutputMap,
                                     this->Invocation.GetInputDomain()));
  }

private:
  typename std::remove_const<WType>::type Worklet;
  IType Invocation;
};

template <typename WType, typename Hints>
class TaskBasic3D<WType, viskores::internal::NullType, Hints> : public viskores::exec::TaskBase
{
  VISKORES_IS_HINT_LIST(Hints);

public:
  explicit TaskBasic3D(const WType& worklet)
    : Worklet(worklet)
  {
  }

  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }

  VISKORES_EXEC
  void operator()(viskores::Id3 idx, viskores::Id) const { this->Worklet(idx); }

private:
  typename std::remove_const<WType>::type Worklet;
};
}
}
}
} // viskores::exec::kokkos::internal

#endif //viskores_exec_kokkos_internal_TaskBasic_h
