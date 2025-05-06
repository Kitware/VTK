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
#ifndef viskores_cont_openmp_internal_DeviceAdapterAlgorithmOpenMP_h
#define viskores_cont_openmp_internal_DeviceAdapterAlgorithmOpenMP_h

#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/Error.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/internal/DeviceAdapterAlgorithmGeneral.h>

#include <viskores/cont/openmp/internal/DeviceAdapterTagOpenMP.h>
#include <viskores/cont/openmp/internal/FunctorsOpenMP.h>
#include <viskores/cont/openmp/internal/ParallelScanOpenMP.h>
#include <viskores/cont/openmp/internal/ParallelSortOpenMP.h>
#include <viskores/exec/openmp/internal/TaskTilingOpenMP.h>

#include <omp.h>

#include <algorithm>
#include <type_traits>

namespace viskores
{
namespace cont
{

template <>
struct DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagOpenMP>
  : viskores::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagOpenMP>,
      viskores::cont::DeviceAdapterTagOpenMP>
{
  using DevTag = DeviceAdapterTagOpenMP;

public:
  template <typename T, typename U, class CIn, class COut>
  VISKORES_CONT static void Copy(const viskores::cont::ArrayHandle<T, CIn>& input,
                                 viskores::cont::ArrayHandle<U, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    using namespace viskores::cont::openmp;

    const viskores::Id inSize = input.GetNumberOfValues();
    if (inSize == 0)
    {
      output.Allocate(0);
      return;
    }
    viskores::cont::Token token;
    auto inputPortal = input.PrepareForInput(DevTag(), token);
    auto outputPortal = output.PrepareForOutput(inSize, DevTag(), token);
    CopyHelper(inputPortal, outputPortal, 0, 0, inSize);
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

    using namespace viskores::cont::openmp;

    viskores::Id inSize = input.GetNumberOfValues();
    if (inSize == 0)
    {
      output.Allocate(0);
      return;
    }
    viskores::cont::Token token;
    auto inputPortal = input.PrepareForInput(DevTag(), token);
    auto stencilPortal = stencil.PrepareForInput(DevTag(), token);
    auto outputPortal = output.PrepareForOutput(inSize, DevTag(), token);

    auto inIter = viskores::cont::ArrayPortalToIteratorBegin(inputPortal);
    auto stencilIter = viskores::cont::ArrayPortalToIteratorBegin(stencilPortal);
    auto outIter = viskores::cont::ArrayPortalToIteratorBegin(outputPortal);

    CopyIfHelper helper;
    helper.Initialize(inSize, sizeof(T));

    VISKORES_OPENMP_DIRECTIVE(parallel default(shared))
    {
      VISKORES_OPENMP_DIRECTIVE(for schedule(static))
      for (viskores::Id i = 0; i < helper.NumChunks; ++i)
      {
        helper.CopyIf(inIter, stencilIter, outIter, unary_predicate, i);
      }
    }

    viskores::Id numValues = helper.Reduce(outIter);
    token.DetachFromAll();
    output.Allocate(numValues, viskores::CopyFlag::On);
  }


  template <typename T, typename U, class CIn, class COut>
  VISKORES_CONT static bool CopySubRange(const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::Id inputStartIndex,
                                         viskores::Id numberOfValuesToCopy,
                                         viskores::cont::ArrayHandle<U, COut>& output,
                                         viskores::Id outputIndex = 0)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    using namespace viskores::cont::openmp;

    const viskores::Id inSize = input.GetNumberOfValues();

    // Check if the ranges overlap and fail if they do.
    if (input == output &&
        ((outputIndex >= inputStartIndex && outputIndex < inputStartIndex + numberOfValuesToCopy) ||
         (inputStartIndex >= outputIndex && inputStartIndex < outputIndex + numberOfValuesToCopy)))
    {
      return false;
    }

    if (inputStartIndex < 0 || numberOfValuesToCopy < 0 || outputIndex < 0 ||
        inputStartIndex >= inSize)
    { //invalid parameters
      return false;
    }

    //determine if the numberOfElementsToCopy needs to be reduced
    if (inSize < (inputStartIndex + numberOfValuesToCopy))
    { //adjust the size
      numberOfValuesToCopy = (inSize - inputStartIndex);
    }

    const viskores::Id outSize = output.GetNumberOfValues();
    const viskores::Id copyOutEnd = outputIndex + numberOfValuesToCopy;
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
    auto inputPortal = input.PrepareForInput(DevTag(), token);
    auto outputPortal = output.PrepareForInPlace(DevTag(), token);

    CopyHelper(inputPortal, outputPortal, inputStartIndex, outputIndex, numberOfValuesToCopy);

    return true;
  }

  template <typename T, typename U, class CIn>
  VISKORES_CONT static U Reduce(const viskores::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    return Reduce(input, initialValue, viskores::Add());
  }

  template <typename T, typename U, class CIn, class BinaryFunctor>
  VISKORES_CONT static U Reduce(const viskores::cont::ArrayHandle<T, CIn>& input,
                                U initialValue,
                                BinaryFunctor binary_functor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    using namespace viskores::cont::openmp;

    viskores::cont::Token token;
    auto portal = input.PrepareForInput(DevTag(), token);
    const OpenMPReductionSupported<typename std::decay<U>::type> fastPath;

    return ReduceHelper::Execute(portal, initialValue, binary_functor, fastPath);
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
                                        BinaryFunctor func)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    openmp::ReduceByKeyHelper(keys, values, keys_output, values_output, func);
  }

  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    return ScanInclusive(input, output, viskores::Add());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binaryFunctor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    if (input.GetNumberOfValues() <= 0)
    {
      return viskores::TypeTraits<T>::ZeroInitialization();
    }

    viskores::cont::Token token;
    using InPortalT = decltype(input.PrepareForInput(DevTag(), token));
    using OutPortalT = decltype(output.PrepareForOutput(0, DevTag(), token));
    using Impl = openmp::ScanInclusiveHelper<InPortalT, OutPortalT, BinaryFunctor>;

    viskores::Id numVals = input.GetNumberOfValues();
    Impl impl(input.PrepareForInput(DevTag(), token),
              output.PrepareForOutput(numVals, DevTag(), token),
              binaryFunctor);

    return impl.Execute(viskores::Id2(0, numVals));
  }

  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    return ScanExclusive(
      input, output, viskores::Add(), viskores::TypeTraits<T>::ZeroInitialization());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binaryFunctor,
                                       const T& initialValue)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    if (input.GetNumberOfValues() <= 0)
    {
      return initialValue;
    }

    viskores::cont::Token token;
    using InPortalT = decltype(input.PrepareForInput(DevTag(), token));
    using OutPortalT = decltype(output.PrepareForOutput(0, DevTag(), token));
    using Impl = openmp::ScanExclusiveHelper<InPortalT, OutPortalT, BinaryFunctor>;

    viskores::Id numVals = input.GetNumberOfValues();
    Impl impl(input.PrepareForInput(DevTag(), token),
              output.PrepareForOutput(numVals, DevTag(), token),
              binaryFunctor,
              initialValue);

    return impl.Execute(viskores::Id2(0, numVals));
  }

  /// \brief Unstable ascending sort of input array.
  ///
  /// Sorts the contents of \c values so that they in ascending value. Doesn't
  /// guarantee stability
  ///
  template <typename T, class Storage>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Storage>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    Sort(values, viskores::SortLess());
  }

  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Storage>& values,
                                 BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    openmp::sort::parallel_sort(values, binary_compare);
  }

  template <typename T, typename U, class StorageT, class StorageU>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    SortByKey(keys, values, std::less<T>());
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values,
                                      BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    openmp::sort::parallel_sort_bykey(keys, values, binary_compare);
  }

  template <typename T, class Storage>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    Unique(values, std::equal_to<T>());
  }

  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values,
                                   BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;
    auto portal = values.PrepareForInPlace(DevTag(), token);
    auto iter = viskores::cont::ArrayPortalToIteratorBegin(portal);

    using IterT = typename std::decay<decltype(iter)>::type;
    using Uniqifier = openmp::UniqueHelper<IterT, BinaryCompare>;

    Uniqifier uniquifier(iter, portal.GetNumberOfValues(), binary_compare);
    viskores::Id outSize = uniquifier.Execute();
    token.DetachFromAll();
    values.Allocate(outSize, viskores::CopyFlag::On);
  }

  VISKORES_CONT_EXPORT static void ScheduleTask(
    viskores::exec::openmp::internal::TaskTiling1D& functor,
    viskores::Id size);
  VISKORES_CONT_EXPORT static void ScheduleTask(
    viskores::exec::openmp::internal::TaskTiling3D& functor,
    viskores::Id3 size);

  template <typename Hints, typename FunctorType>
  VISKORES_CONT static inline void Schedule(Hints, FunctorType functor, viskores::Id numInstances)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::exec::openmp::internal::TaskTiling1D kernel(functor);
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
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::exec::openmp::internal::TaskTiling3D kernel(functor);
    ScheduleTask(kernel, rangeMax);
  }

  template <typename FunctorType>
  VISKORES_CONT static inline void Schedule(FunctorType&& functor, viskores::Id3 rangeMax)
  {
    Schedule(viskores::cont::internal::HintList<>{}, functor, rangeMax);
  }

  VISKORES_CONT static void Synchronize()
  {
    // Nothing to do. This device schedules all of its operations using a
    // split/join paradigm. This means that the if the control thread is
    // calling this method, then nothing should be running in the execution
    // environment.
  }
};

template <>
class DeviceTaskTypes<viskores::cont::DeviceAdapterTagOpenMP>
{
public:
  template <typename Hints, typename WorkletType, typename InvocationType>
  static viskores::exec::openmp::internal::TaskTiling1D MakeTask(const WorkletType& worklet,
                                                                 const InvocationType& invocation,
                                                                 viskores::Id,
                                                                 Hints = Hints{})
  {
    // Currently ignoring hints.
    return viskores::exec::openmp::internal::TaskTiling1D(worklet, invocation);
  }

  template <typename Hints, typename WorkletType, typename InvocationType>
  static viskores::exec::openmp::internal::TaskTiling3D MakeTask(const WorkletType& worklet,
                                                                 const InvocationType& invocation,
                                                                 viskores::Id3,
                                                                 Hints = Hints{})
  {
    // Currently ignoring hints.
    return viskores::exec::openmp::internal::TaskTiling3D(worklet, invocation);
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

#endif //viskores_cont_openmp_internal_DeviceAdapterAlgorithmOpenMP_h
