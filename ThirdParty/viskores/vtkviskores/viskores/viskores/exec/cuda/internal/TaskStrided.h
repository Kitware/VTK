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
#ifndef viskores_exec_cuda_internal_TaskStrided_h
#define viskores_exec_cuda_internal_TaskStrided_h

#include <viskores/exec/TaskBase.h>

#include <viskores/cont/cuda/internal/CudaAllocator.h>

//Todo: rename this header to TaskInvokeWorkletDetail.h
#include <viskores/exec/internal/WorkletInvokeFunctorDetail.h>

namespace viskores
{
namespace exec
{
namespace cuda
{
namespace internal
{

template <typename WType>
void TaskStridedSetErrorBuffer(void* w, const viskores::exec::internal::ErrorMessageBuffer& buffer)
{
  using WorkletType = typename std::remove_cv<WType>::type;
  WorkletType* const worklet = static_cast<WorkletType*>(w);
  worklet->SetErrorMessageBuffer(buffer);
}

class TaskStrided : public viskores::exec::TaskBase
{
public:
  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer& buffer)
  {
    (void)buffer;
    this->SetErrorBufferFunction(this->WPtr, buffer);
  }

protected:
  void* WPtr = nullptr;

  using SetErrorBufferSignature = void (*)(void*,
                                           const viskores::exec::internal::ErrorMessageBuffer&);
  SetErrorBufferSignature SetErrorBufferFunction = nullptr;
};

template <typename WType, typename IType, typename Hints>
class TaskStrided1D : public TaskStrided
{
  VISKORES_IS_HINT_LIST(Hints);

public:
  TaskStrided1D(const WType& worklet, const IType& invocation)
    : TaskStrided()
    , Worklet(worklet)
    , Invocation(invocation)
  {
    this->SetErrorBufferFunction = &TaskStridedSetErrorBuffer<WType>;
    //Bind the Worklet to void*
    this->WPtr = (void*)&this->Worklet;
  }

  VISKORES_EXEC
  void operator()(viskores::Id start, viskores::Id end, viskores::Id inc) const
  {
    for (viskores::Id index = start; index < end; index += inc)
    {
      //Todo: rename this function to DoTaskInvokeWorklet
      viskores::exec::internal::detail::DoWorkletInvokeFunctor(
        this->Worklet,
        this->Invocation,
        this->Worklet.GetThreadIndices(index,
                                       this->Invocation.OutputToInputMap,
                                       this->Invocation.VisitArray,
                                       this->Invocation.ThreadToOutputMap,
                                       this->Invocation.GetInputDomain()));
    }
  }

private:
  typename std::remove_const<WType>::type Worklet;
  // This is held by by value so that when we transfer the invocation object
  // over to CUDA it gets properly copied to the device. While we want to
  // hold by reference to reduce the number of copies, it is not possible
  // currently.
  const IType Invocation;
};

template <typename WType, typename Hints>
class TaskStrided1D<WType, viskores::internal::NullType, Hints> : public TaskStrided
{
  VISKORES_IS_HINT_LIST(Hints);

public:
  TaskStrided1D(WType& worklet)
    : TaskStrided()
    , Worklet(worklet)
  {
    this->SetErrorBufferFunction = &TaskStridedSetErrorBuffer<WType>;
    //Bind the Worklet to void*
    this->WPtr = (void*)&this->Worklet;
  }

  VISKORES_EXEC
  void operator()(viskores::Id start, viskores::Id end, viskores::Id inc) const
  {
    for (viskores::Id index = start; index < end; index += inc)
    {
      this->Worklet(index);
    }
  }

private:
  typename std::remove_const<WType>::type Worklet;
};

template <typename WType, typename IType, typename Hints>
class TaskStrided3D : public TaskStrided
{
  VISKORES_IS_HINT_LIST(Hints);

public:
  TaskStrided3D(const WType& worklet, const IType& invocation)
    : TaskStrided()
    , Worklet(worklet)
    , Invocation(invocation)
  {
    this->SetErrorBufferFunction = &TaskStridedSetErrorBuffer<WType>;
    //Bind the Worklet to void*
    this->WPtr = (void*)&this->Worklet;
  }

  VISKORES_EXEC
  void operator()(const viskores::Id3& size,
                  viskores::Id start,
                  viskores::Id end,
                  viskores::Id inc,
                  viskores::Id j,
                  viskores::Id k) const
  {
    viskores::Id3 index(start, j, k);
    auto threadIndex1D = index[0] + size[0] * (index[1] + size[1] * index[2]);
    for (viskores::Id i = start; i < end; i += inc, threadIndex1D += inc)
    {
      index[0] = i;
      //Todo: rename this function to DoTaskInvokeWorklet
      viskores::exec::internal::detail::DoWorkletInvokeFunctor(
        this->Worklet,
        this->Invocation,
        this->Worklet.GetThreadIndices(threadIndex1D,
                                       index,
                                       this->Invocation.OutputToInputMap,
                                       this->Invocation.VisitArray,
                                       this->Invocation.ThreadToOutputMap,
                                       this->Invocation.GetInputDomain()));
    }
  }

private:
  typename std::remove_const<WType>::type Worklet;
  // This is held by by value so that when we transfer the invocation object
  // over to CUDA it gets properly copied to the device. While we want to
  // hold by reference to reduce the number of copies, it is not possible
  // currently.
  const IType Invocation;
};

template <typename WType, typename Hints>
class TaskStrided3D<WType, viskores::internal::NullType, Hints> : public TaskStrided
{
  VISKORES_IS_HINT_LIST(Hints);

public:
  TaskStrided3D(WType& worklet)
    : TaskStrided()
    , Worklet(worklet)
  {
    this->SetErrorBufferFunction = &TaskStridedSetErrorBuffer<WType>;
    //Bind the Worklet to void*
    this->WPtr = (void*)&this->Worklet;
  }

  VISKORES_EXEC
  void operator()(const viskores::Id3& size,
                  viskores::Id start,
                  viskores::Id end,
                  viskores::Id inc,
                  viskores::Id j,
                  viskores::Id k) const
  {
    viskores::Id3 index(start, j, k);
    for (viskores::Id i = start; i < end; i += inc)
    {
      index[0] = i;
      this->Worklet(index);
    }
  }

private:
  typename std::remove_const<WType>::type Worklet;
};
}
}
}
} // viskores::exec::cuda::internal

#endif //viskores_exec_cuda_internal_TaskStrided_h
