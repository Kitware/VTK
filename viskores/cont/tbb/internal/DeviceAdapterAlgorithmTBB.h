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
#ifndef viskores_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h
#define viskores_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleZip.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/ErrorExecution.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <viskores/cont/internal/IteratorFromArrayPortal.h>
#include <viskores/cont/tbb/internal/DeviceAdapterTagTBB.h>
#include <viskores/cont/tbb/internal/FunctorsTBB.h>
#include <viskores/cont/tbb/internal/ParallelSortTBB.h>

#include <viskores/exec/tbb/internal/TaskTiling.h>

namespace viskores
{
namespace cont
{

template <>
struct DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagTBB>
  : viskores::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagTBB>,
      viskores::cont::DeviceAdapterTagTBB>
{
public:
  template <typename T, typename U, class CIn, class COut>
  VISKORES_CONT static void Copy(const viskores::cont::ArrayHandle<T, CIn>& input,
                                 viskores::cont::ArrayHandle<U, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;

    const viskores::Id inSize = input.GetNumberOfValues();
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagTBB(), token);
    auto outputPortal = output.PrepareForOutput(inSize, DeviceAdapterTagTBB(), token);

    tbb::CopyPortals(inputPortal, outputPortal, 0, 0, inSize);
  }

  template <typename T, typename U, class CIn, class CStencil, class COut>
  VISKORES_CONT static void CopyIf(const viskores::cont::ArrayHandle<T, CIn>& input,
                                   const viskores::cont::ArrayHandle<U, CStencil>& stencil,
                                   viskores::cont::ArrayHandle<T, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    ::viskores::NotZeroInitialized unary_predicate;
    CopyIf(input, stencil, output, unary_predicate);
  }

  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  VISKORES_CONT static void CopyIf(const viskores::cont::ArrayHandle<T, CIn>& input,
                                   const viskores::cont::ArrayHandle<U, CStencil>& stencil,
                                   viskores::cont::ArrayHandle<T, COut>& output,
                                   UnaryPredicate unary_predicate)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;

    viskores::Id inputSize = input.GetNumberOfValues();
    VISKORES_ASSERT(inputSize == stencil.GetNumberOfValues());
    viskores::Id outputSize =
      tbb::CopyIfPortals(input.PrepareForInput(DeviceAdapterTagTBB(), token),
                         stencil.PrepareForInput(DeviceAdapterTagTBB(), token),
                         output.PrepareForOutput(inputSize, DeviceAdapterTagTBB(), token),
                         unary_predicate);
    token.DetachFromAll();
    output.Allocate(outputSize, viskores::CopyFlag::On);
  }

  template <typename T, typename U, class CIn, class COut>
  VISKORES_CONT static bool CopySubRange(const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::Id inputStartIndex,
                                         viskores::Id numberOfElementsToCopy,
                                         viskores::cont::ArrayHandle<U, COut>& output,
                                         viskores::Id outputIndex = 0)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id inSize = input.GetNumberOfValues();

    // Check if the ranges overlap and fail if they do.
    if (input == output &&
        ((outputIndex >= inputStartIndex &&
          outputIndex < inputStartIndex + numberOfElementsToCopy) ||
         (inputStartIndex >= outputIndex &&
          inputStartIndex < outputIndex + numberOfElementsToCopy)))
    {
      return false;
    }

    if (inputStartIndex < 0 || numberOfElementsToCopy < 0 || outputIndex < 0 ||
        inputStartIndex >= inSize)
    { //invalid parameters
      return false;
    }

    //determine if the numberOfElementsToCopy needs to be reduced
    if (inSize < (inputStartIndex + numberOfElementsToCopy))
    { //adjust the size
      numberOfElementsToCopy = (inSize - inputStartIndex);
    }

    const viskores::Id outSize = output.GetNumberOfValues();
    const viskores::Id copyOutEnd = outputIndex + numberOfElementsToCopy;
    if (outSize < copyOutEnd)
    { //output is not large enough
      if (outSize == 0)
      { //since output has nothing, just need to allocate to correct length
        output.Allocate(copyOutEnd);
      }
      else
      { //we currently have data in this array, so preserve it in the new
        //resized array
        viskores::cont::ArrayHandle<U, COut> temp;
        temp.Allocate(copyOutEnd);
        CopySubRange(output, 0, outSize, temp);
        output = temp;
      }
    }

    viskores::cont::Token token;
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagTBB(), token);
    auto outputPortal = output.PrepareForInPlace(DeviceAdapterTagTBB(), token);

    tbb::CopyPortals(
      inputPortal, outputPortal, inputStartIndex, outputIndex, numberOfElementsToCopy);

    return true;
  }

  template <typename T, typename U, class CIn>
  VISKORES_CONT static auto Reduce(const viskores::cont::ArrayHandle<T, CIn>& input, U initialValue)
    -> decltype(Reduce(input, initialValue, viskores::Add{}))
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    return Reduce(input, initialValue, viskores::Add());
  }

  template <typename T, typename U, class CIn, class BinaryFunctor>
  VISKORES_CONT static auto Reduce(const viskores::cont::ArrayHandle<T, CIn>& input,
                                   U initialValue,
                                   BinaryFunctor binary_functor)
    -> decltype(tbb::ReducePortals(input.ReadPortal(), initialValue, binary_functor))
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;
    return tbb::ReducePortals(input.PrepareForInput(viskores::cont::DeviceAdapterTagTBB(), token),
                              initialValue,
                              binary_functor);
  }

  template <typename T,
            typename U,
            class CKeyIn,
            class CValIn,
            class CKeyOut,
            class CValOut,
            class BinaryFunctor>
  VISKORES_CONT static void ReduceByKey(const viskores::cont::ArrayHandle<T, CKeyIn>& keys,
                                        const viskores::cont::ArrayHandle<U, CValIn>& values,
                                        viskores::cont::ArrayHandle<T, CKeyOut>& keys_output,
                                        viskores::cont::ArrayHandle<U, CValOut>& values_output,
                                        BinaryFunctor binary_functor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;

    viskores::Id inputSize = keys.GetNumberOfValues();
    VISKORES_ASSERT(inputSize == values.GetNumberOfValues());
    viskores::Id outputSize = tbb::ReduceByKeyPortals(
      keys.PrepareForInput(DeviceAdapterTagTBB(), token),
      values.PrepareForInput(DeviceAdapterTagTBB(), token),
      keys_output.PrepareForOutput(inputSize, DeviceAdapterTagTBB(), token),
      values_output.PrepareForOutput(inputSize, DeviceAdapterTagTBB(), token),
      binary_functor);
    token.DetachFromAll();
    keys_output.Allocate(outputSize, viskores::CopyFlag::On);
    values_output.Allocate(outputSize, viskores::CopyFlag::On);
  }

  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;
    return tbb::ScanInclusivePortals(
      input.PrepareForInput(viskores::cont::DeviceAdapterTagTBB(), token),
      output.PrepareForOutput(
        input.GetNumberOfValues(), viskores::cont::DeviceAdapterTagTBB(), token),
      viskores::Add());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binary_functor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;
    return tbb::ScanInclusivePortals(
      input.PrepareForInput(viskores::cont::DeviceAdapterTagTBB(), token),
      output.PrepareForOutput(
        input.GetNumberOfValues(), viskores::cont::DeviceAdapterTagTBB(), token),
      binary_functor);
  }

  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;
    return tbb::ScanExclusivePortals(
      input.PrepareForInput(viskores::cont::DeviceAdapterTagTBB(), token),
      output.PrepareForOutput(
        input.GetNumberOfValues(), viskores::cont::DeviceAdapterTagTBB(), token),
      viskores::Add(),
      viskores::TypeTraits<T>::ZeroInitialization());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binary_functor,
                                       const T& initialValue)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;
    return tbb::ScanExclusivePortals(
      input.PrepareForInput(viskores::cont::DeviceAdapterTagTBB(), token),
      output.PrepareForOutput(
        input.GetNumberOfValues(), viskores::cont::DeviceAdapterTagTBB(), token),
      binary_functor,
      initialValue);
  }

  VISKORES_CONT_EXPORT static void ScheduleTask(
    viskores::exec::tbb::internal::TaskTiling1D& functor,
    viskores::Id size);
  VISKORES_CONT_EXPORT static void ScheduleTask(
    viskores::exec::tbb::internal::TaskTiling3D& functor,
    viskores::Id3 size);

  template <typename Hints, typename FunctorType>
  VISKORES_CONT static inline void Schedule(Hints, FunctorType functor, viskores::Id numInstances)
  {
    VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf,
                       "Schedule TBB 1D: '%s'",
                       viskores::cont::TypeToString(functor).c_str());

    viskores::exec::tbb::internal::TaskTiling1D kernel(functor);
    ScheduleTask(kernel, numInstances);
  }

  template <typename FunctorType>
  VISKORES_CONT static inline void Schedule(FunctorType&& functor, viskores::Id numInstances)
  {
    Schedule(viskores::cont::internal::HintList<>{}, functor, numInstances);
  }

  template <typename Hints, typename FunctorType>
  VISKORES_CONT static inline void Schedule(Hints, FunctorType functor, viskores::Id3 rangeMax)
  {
    VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf,
                       "Schedule TBB 3D: '%s'",
                       viskores::cont::TypeToString(functor).c_str());

    viskores::exec::tbb::internal::TaskTiling3D kernel(functor);
    ScheduleTask(kernel, rangeMax);
  }

  template <typename FunctorType>
  VISKORES_CONT static inline void Schedule(FunctorType&& functor, viskores::Id3 rangeMax)
  {
    Schedule(viskores::cont::internal::HintList<>{}, functor, rangeMax);
  }

  //1. We need functions for each of the following


  template <typename T, class Container>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Container>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    //this is required to get sort to work with zip handles
    std::less<T> lessOp;
    viskores::cont::tbb::sort::parallel_sort(values, lessOp);
  }

  template <typename T, class Container, class BinaryCompare>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Container>& values,
                                 BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::tbb::sort::parallel_sort(values, binary_compare);
  }

  template <typename T, typename U, class StorageT, class StorageU>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::tbb::sort::parallel_sort_bykey(keys, values, std::less<T>());
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values,
                                      BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::tbb::sort::parallel_sort_bykey(keys, values, binary_compare);
  }

  template <typename T, class Storage>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values)
  {
    Unique(values, std::equal_to<T>());
  }

  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values,
                                   BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id outputSize;
    {
      viskores::cont::Token token;
      outputSize =
        tbb::UniquePortals(values.PrepareForInPlace(DeviceAdapterTagTBB(), token), binary_compare);
    }
    values.Allocate(outputSize, viskores::CopyFlag::On);
  }

  VISKORES_CONT static void Synchronize()
  {
    // Nothing to do. This device schedules all of its operations using a
    // split/join paradigm. This means that the if the control threaad is
    // calling this method, then nothing should be running in the execution
    // environment.
  }
};

/// TBB contains its own high resolution timer.
///
template <>
class DeviceAdapterTimerImplementation<viskores::cont::DeviceAdapterTagTBB>
{
public:
  VISKORES_CONT DeviceAdapterTimerImplementation() { this->Reset(); }

  VISKORES_CONT void Reset()
  {
    this->StartReady = false;
    this->StopReady = false;
  }

  VISKORES_CONT void Start()
  {
    this->Reset();
    this->StartTime = this->GetCurrentTime();
    this->StartReady = true;
  }

  VISKORES_CONT void Stop()
  {
    this->StopTime = this->GetCurrentTime();
    this->StopReady = true;
  }

  VISKORES_CONT bool Started() const { return this->StartReady; }

  VISKORES_CONT bool Stopped() const { return this->StopReady; }

  VISKORES_CONT bool Ready() const { return true; }

  VISKORES_CONT viskores::Float64 GetElapsedTime() const
  {
    assert(this->StartReady);
    if (!this->StartReady)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Error,
                     "Start() function should be called first then trying to call Stop() and"
                     " GetElapsedTime().");
      return 0;
    }

    ::tbb::tick_count startTime = this->StartTime;
    ::tbb::tick_count stopTime = this->StopReady ? this->StopTime : this->GetCurrentTime();

    ::tbb::tick_count::interval_t elapsedTime = stopTime - startTime;

    return static_cast<viskores::Float64>(elapsedTime.seconds());
  }

  VISKORES_CONT::tbb::tick_count GetCurrentTime() const
  {
    viskores::cont::DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagTBB>::Synchronize();
    return ::tbb::tick_count::now();
  }

private:
  bool StartReady;
  bool StopReady;
  ::tbb::tick_count StartTime;
  ::tbb::tick_count StopTime;
};

template <>
class DeviceTaskTypes<viskores::cont::DeviceAdapterTagTBB>
{
public:
  template <typename Hints, typename WorkletType, typename InvocationType>
  static viskores::exec::tbb::internal::TaskTiling1D MakeTask(WorkletType& worklet,
                                                              InvocationType& invocation,
                                                              viskores::Id,
                                                              Hints = Hints{})
  {
    // Currently ignoring hints.
    return viskores::exec::tbb::internal::TaskTiling1D(worklet, invocation);
  }

  template <typename Hints, typename WorkletType, typename InvocationType>
  static viskores::exec::tbb::internal::TaskTiling3D MakeTask(WorkletType& worklet,
                                                              InvocationType& invocation,
                                                              viskores::Id3,
                                                              Hints = Hints{})
  {
    // Currently ignoring hints.
    return viskores::exec::tbb::internal::TaskTiling3D(worklet, invocation);
  }

  template <typename WorkletType, typename InvocationType, typename RangeType>
  VISKORES_CONT static auto MakeTask(WorkletType& worklet,
                                     InvocationType& invocation,
                                     const RangeType& range)
  {
    return MakeTask<viskores::cont::internal::HintList<>>(worklet, invocation, range);
  }
};
}
} // namespace viskores::cont

#endif //viskores_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h
