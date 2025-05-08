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
#ifndef viskores_exec_internal_TaskSingular_h
#define viskores_exec_internal_TaskSingular_h

#include <viskores/internal/Invocation.h>

#include <viskores/exec/TaskBase.h>

#include <viskores/exec/arg/Fetch.h>

//Todo: rename this header to TaskSingularDetail.h
#include <viskores/exec/internal/WorkletInvokeFunctorDetail.h>

namespace viskores
{
namespace exec
{
namespace internal
{

// TaskSingular represents an execution pattern for a worklet
// that is best expressed in terms of single dimension iteration space. Inside
// this single dimension no order is preferred.
//
//
template <typename WorkletType, typename InvocationType>
class TaskSingular : public viskores::exec::TaskBase
{
public:
  VISKORES_CONT
  TaskSingular(const WorkletType& worklet, const InvocationType& invocation)
    : Worklet(worklet)
    , Invocation(invocation)
  {
  }

  VISKORES_CONT
  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC void operator()(T index) const
  {
    //Todo: rename this function to DoTaskSingular
    detail::DoWorkletInvokeFunctor(
      this->Worklet,
      this->Invocation,
      this->Worklet.GetThreadIndices(index,
                                     this->Invocation.OutputToInputMap,
                                     this->Invocation.VisitArray,
                                     this->Invocation.ThreadToOutputMap,
                                     this->Invocation.GetInputDomain()));
  }

private:
  typename std::remove_const<WorkletType>::type Worklet;
  // This is held by by value so that when we transfer the invocation object
  // over to CUDA it gets properly copied to the device. While we want to
  // hold by reference to reduce the number of copies, it is not possible
  // currently.
  const InvocationType Invocation;
};
}
}
} // viskores::exec::internal

#endif //viskores_exec_internal_TaskSingular_h
