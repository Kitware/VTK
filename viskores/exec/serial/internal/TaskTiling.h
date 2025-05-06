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

#ifndef viskores_exec_serial_internal_TaskTiling_h
#define viskores_exec_serial_internal_TaskTiling_h

#include <viskores/exec/TaskBase.h>

//Todo: rename this header to TaskInvokeWorkletDetail.h
#include <viskores/exec/internal/WorkletInvokeFunctorDetail.h>

namespace viskores
{
namespace exec
{
namespace serial
{
namespace internal
{

template <typename WType>
VISKORES_NEVER_EXPORT void TaskTilingSetErrorBuffer(
  void* w,
  const viskores::exec::internal::ErrorMessageBuffer& buffer)
{
  using WorkletType = typename std::remove_cv<WType>::type;
  WorkletType* const worklet = static_cast<WorkletType*>(w);
  worklet->SetErrorMessageBuffer(buffer);
}

template <typename WType, typename IType>
VISKORES_NEVER_EXPORT void TaskTiling1DExecute(void* w,
                                               void* const v,
                                               viskores::Id start,
                                               viskores::Id end)
{
  using WorkletType = typename std::remove_cv<WType>::type;
  using InvocationType = typename std::remove_cv<IType>::type;

  WorkletType const* const worklet = static_cast<WorkletType*>(w);
  InvocationType const* const invocation = static_cast<InvocationType*>(v);

  for (viskores::Id index = start; index < end; ++index)
  {
    //Todo: rename this function to DoTaskInvokeWorklet
    viskores::exec::internal::detail::DoWorkletInvokeFunctor(
      *worklet,
      *invocation,
      worklet->GetThreadIndices(index,
                                invocation->OutputToInputMap,
                                invocation->VisitArray,
                                invocation->ThreadToOutputMap,
                                invocation->GetInputDomain()));
  }
}

template <typename FType>
VISKORES_NEVER_EXPORT void FunctorTiling1DExecute(void* f,
                                                  void* const,
                                                  viskores::Id start,
                                                  viskores::Id end)
{
  using FunctorType = typename std::remove_cv<FType>::type;
  FunctorType const* const functor = static_cast<FunctorType*>(f);

  for (viskores::Id index = start; index < end; ++index)
  {
    functor->operator()(index);
  }
}

template <typename WType, typename IType>
VISKORES_NEVER_EXPORT void TaskTiling3DExecute(void* w,
                                               void* const v,
                                               const viskores::Id3& maxSize,
                                               viskores::Id istart,
                                               viskores::Id iend,
                                               viskores::Id j,
                                               viskores::Id k)
{
  using WorkletType = typename std::remove_cv<WType>::type;
  using InvocationType = typename std::remove_cv<IType>::type;

  WorkletType const* const worklet = static_cast<WorkletType*>(w);
  InvocationType const* const invocation = static_cast<InvocationType*>(v);

  viskores::Id3 index(istart, j, k);
  auto threadIndex1D = index[0] + maxSize[0] * (index[1] + maxSize[1] * index[2]);
  for (viskores::Id i = istart; i < iend; ++i, ++threadIndex1D)
  {
    index[0] = i;
    //Todo: rename this function to DoTaskInvokeWorklet
    viskores::exec::internal::detail::DoWorkletInvokeFunctor(
      *worklet,
      *invocation,
      worklet->GetThreadIndices(threadIndex1D,
                                index,
                                invocation->OutputToInputMap,
                                invocation->VisitArray,
                                invocation->ThreadToOutputMap,
                                invocation->GetInputDomain()));
  }
}

template <typename FType>
VISKORES_NEVER_EXPORT void FunctorTiling3DExecute(void* f,
                                                  void* const,
                                                  const viskores::Id3& viskoresNotUsed(maxSize),
                                                  viskores::Id istart,
                                                  viskores::Id iend,
                                                  viskores::Id j,
                                                  viskores::Id k)
{
  using FunctorType = typename std::remove_cv<FType>::type;
  FunctorType const* const functor = static_cast<FunctorType*>(f);

  viskores::Id3 index(istart, j, k);
  for (viskores::Id i = istart; i < iend; ++i)
  {
    index[0] = i;
    functor->operator()(index);
  }
}

// TaskTiling1D represents an execution pattern for a worklet
// that is best expressed in terms of single dimension iteration space. TaskTiling1D
// also states that for best performance a linear consecutive range of values
// should be given to the worklet for optimal performance.
//
// Note: The worklet and invocation must have a lifetime that is at least
// as long as the Task
class VISKORES_NEVER_EXPORT TaskTiling1D : public viskores::exec::TaskBase
{
public:
  TaskTiling1D()
    : Worklet(nullptr)
    , Invocation(nullptr)
  {
  }

  /// This constructor supports general functors that which have a call
  /// operator with the signature:
  ///   operator()(viskores::Id)
  template <typename FunctorType>
  TaskTiling1D(FunctorType& functor)
    : Worklet(nullptr)
    , Invocation(nullptr)
    , ExecuteFunction(nullptr)
    , SetErrorBufferFunction(nullptr)
  {
    //Setup the execute and set error buffer function pointers
    this->ExecuteFunction = &FunctorTiling1DExecute<FunctorType>;
    this->SetErrorBufferFunction = &TaskTilingSetErrorBuffer<FunctorType>;

    //Bind the Worklet to void*
    this->Worklet = (void*)&functor;
  }

  /// This constructor supports any viskores worklet and the associated invocation
  /// parameters that go along with it
  template <typename WorkletType, typename InvocationType>
  TaskTiling1D(WorkletType& worklet, InvocationType& invocation)
    : Worklet(nullptr)
    , Invocation(nullptr)
    , ExecuteFunction(nullptr)
    , SetErrorBufferFunction(nullptr)
  {
    //Setup the execute and set error buffer function pointers
    this->ExecuteFunction = &TaskTiling1DExecute<WorkletType, InvocationType>;
    this->SetErrorBufferFunction = &TaskTilingSetErrorBuffer<WorkletType>;

    //Bind the Worklet and Invocation to void*
    this->Worklet = (void*)&worklet;
    this->Invocation = (void*)&invocation;
  }

  /// explicit Copy constructor.
  /// Note this required so that compilers don't use the templated constructor
  /// as the copy constructor which will cause compile issues
  TaskTiling1D(TaskTiling1D& task)
    : Worklet(task.Worklet)
    , Invocation(task.Invocation)
    , ExecuteFunction(task.ExecuteFunction)
    , SetErrorBufferFunction(task.SetErrorBufferFunction)
  {
  }

  TaskTiling1D(TaskTiling1D&& task) = default;

  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->SetErrorBufferFunction(this->Worklet, buffer);
  }

  void operator()(viskores::Id start, viskores::Id end) const
  {
    this->ExecuteFunction(this->Worklet, this->Invocation, start, end);
  }

protected:
  void* Worklet;
  void* Invocation;

  using ExecuteSignature = void (*)(void*, void* const, viskores::Id, viskores::Id);
  ExecuteSignature ExecuteFunction;

  using SetErrorBufferSignature = void (*)(void*,
                                           const viskores::exec::internal::ErrorMessageBuffer&);
  SetErrorBufferSignature SetErrorBufferFunction;
};

// TaskTiling3D represents an execution pattern for a worklet
// that is best expressed in terms of an 3 dimensional iteration space. TaskTiling3D
// also states that for best performance a linear consecutive range of values
// in the X dimension should be given to the worklet for optimal performance.
//
// Note: The worklet and invocation must have a lifetime that is at least
// as long as the Task
class VISKORES_NEVER_EXPORT TaskTiling3D : public viskores::exec::TaskBase
{
public:
  TaskTiling3D()
    : Worklet(nullptr)
    , Invocation(nullptr)
  {
  }

  /// This constructor supports general functors that which have a call
  /// operator with the signature:
  ///   operator()(viskores::Id)
  template <typename FunctorType>
  TaskTiling3D(FunctorType& functor)
    : Worklet(nullptr)
    , Invocation(nullptr)
    , ExecuteFunction(nullptr)
    , SetErrorBufferFunction(nullptr)
  {
    //Setup the execute and set error buffer function pointers
    this->ExecuteFunction = &FunctorTiling3DExecute<FunctorType>;
    this->SetErrorBufferFunction = &TaskTilingSetErrorBuffer<FunctorType>;

    //Bind the Worklet to void*
    this->Worklet = (void*)&functor;
  }

  template <typename WorkletType, typename InvocationType>
  TaskTiling3D(WorkletType& worklet, InvocationType& invocation)
    : Worklet(nullptr)
    , Invocation(nullptr)
    , ExecuteFunction(nullptr)
    , SetErrorBufferFunction(nullptr)
  {
    // Setup the execute and set error buffer function pointers
    this->ExecuteFunction = &TaskTiling3DExecute<WorkletType, InvocationType>;
    this->SetErrorBufferFunction = &TaskTilingSetErrorBuffer<WorkletType>;

    // At this point we bind the Worklet and Invocation to void*
    this->Worklet = (void*)&worklet;
    this->Invocation = (void*)&invocation;
  }

  /// explicit Copy constructor.
  /// Note this required so that compilers don't use the templated constructor
  /// as the copy constructor which will cause compile issues
  TaskTiling3D(TaskTiling3D& task)
    : Worklet(task.Worklet)
    , Invocation(task.Invocation)
    , ExecuteFunction(task.ExecuteFunction)
    , SetErrorBufferFunction(task.SetErrorBufferFunction)
  {
  }

  TaskTiling3D(TaskTiling3D&& task) = default;

  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->SetErrorBufferFunction(this->Worklet, buffer);
  }

  void operator()(const viskores::Id3& maxSize,
                  viskores::Id istart,
                  viskores::Id iend,
                  viskores::Id j,
                  viskores::Id k) const
  {
    this->ExecuteFunction(this->Worklet, this->Invocation, maxSize, istart, iend, j, k);
  }

protected:
  void* Worklet;
  void* Invocation;

  using ExecuteSignature = void (*)(void*,
                                    void* const,
                                    const viskores::Id3&,
                                    viskores::Id,
                                    viskores::Id,
                                    viskores::Id,
                                    viskores::Id);
  ExecuteSignature ExecuteFunction;

  using SetErrorBufferSignature = void (*)(void*,
                                           const viskores::exec::internal::ErrorMessageBuffer&);
  SetErrorBufferSignature SetErrorBufferFunction;
};
}
}
}
} // viskores::exec::serial::internal

#endif //viskores_exec_serial_internal_TaskTiling_h
