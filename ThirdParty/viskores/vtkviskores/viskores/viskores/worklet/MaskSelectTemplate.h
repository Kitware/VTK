//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_worklet_MaskSelectTemplate_h
#define viskores_worklet_MaskSelectTemplate_h

#include <viskores/worklet/MaskSelect.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleView.h>

namespace viskores
{
namespace worklet
{

namespace internal
{

struct MaskSelectReverseOutputToThreadMap : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn outputToThreadMap,
                                FieldIn maskArray,
                                WholeArrayOut threadToOutputMap);
  using ExecutionSignature = void(_1, InputIndex, _2, _3);

  template <typename MaskType, typename ThreadToOutputPortal>
  VISKORES_EXEC void operator()(viskores::Id threadIndex,
                                viskores::Id outputIndex,
                                MaskType mask,
                                ThreadToOutputPortal threadToOutput) const
  {
    if (mask)
    {
      threadToOutput.Set(threadIndex, outputIndex);
    }
  }
};

VISKORES_WORKLET_EXPORT VISKORES_CONT viskores::worklet::MaskSelect::ThreadToOutputMapType
BuildThreadToOutputMapWithFind(viskores::Id numThreads,
                               viskores::cont::ArrayHandle<viskores::Id> outputToThreadMap,
                               viskores::cont::DeviceAdapterId device);

template <typename MaskArrayType>
VISKORES_CONT viskores::worklet::MaskSelect::ThreadToOutputMapType BuildThreadToOutputMapWithCopy(
  viskores::Id numThreads,
  const viskores::cont::ArrayHandle<viskores::Id>& outputToThreadMap,
  const MaskArrayType& maskArray,
  viskores::cont::DeviceAdapterId device)
{
  viskores::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;
  threadToOutputMap.Allocate(numThreads);

  viskores::worklet::DispatcherMapField<MaskSelectReverseOutputToThreadMap> dispatcher;
  dispatcher.SetDevice(device);
  dispatcher.Invoke(outputToThreadMap, maskArray, threadToOutputMap);

  return threadToOutputMap;
}

VISKORES_WORKLET_EXPORT VISKORES_CONT viskores::worklet::MaskSelect::ThreadToOutputMapType
BuildThreadToOutputMapAllOn(viskores::Id numThreads, viskores::cont::DeviceAdapterId device);

template <typename ArrayHandleType>
viskores::worklet::MaskSelect::ThreadToOutputMapType MaskSelectBuild(
  const ArrayHandleType& maskArray,
  viskores::cont::DeviceAdapterId device)
{
  viskores::cont::ArrayHandle<viskores::Id> outputToThreadMap;
  viskores::Id numThreads = viskores::cont::Algorithm::ScanExclusive(
    device, viskores::cont::make_ArrayHandleCast<viskores::Id>(maskArray), outputToThreadMap);
  VISKORES_ASSERT(numThreads <= maskArray.GetNumberOfValues());

  // We have implemented two different ways to compute the thread to output map. The first way is
  // to use a binary search on each thread index into the output map. The second way is to
  // schedule on each output and copy the the index to the thread map. The first way is faster
  // for output sizes that are small relative to the input and also tends to be well load
  // balanced. The second way is faster for larger outputs.
  //
  // The former is obviously faster for one thread and the latter is obviously faster when all
  // outputs have a thread. We have to guess for values in the middle. I'm using if the square of
  // the number of threads is less than the number of outputs because it is easy to compute.
  if (numThreads == maskArray.GetNumberOfValues())
  { //fast path when everything is on
    return BuildThreadToOutputMapAllOn(numThreads, device);
  }
  else if ((numThreads * numThreads) < maskArray.GetNumberOfValues())
  {
    return BuildThreadToOutputMapWithFind(numThreads, outputToThreadMap, device);
  }
  else
  {
    return BuildThreadToOutputMapWithCopy(numThreads, outputToThreadMap, maskArray, device);
  }
}

} // namespace internal

/// @brief A templated version `MaskSelect`.
///
/// To construct a `MaskSelect`, you provide a mask array, which gets processed
/// to construct a lookup array. To prevent multiple recompiles, this is compiled
/// into a library. However, if your mask array is of an atypical type, such as a
/// `viskores::cont::ArrayHandleTransform`, the underlying code will have to copy
/// the array into a form it is familiar with. In this case where you have such
/// an array (and Viskores is warning you about an inefficient array copy), you
/// can use the constructor of this subclass to compile a version of `MaskSelect`
/// directly for your array type.
///
/// Once constructed, this object can (and probably should) be cast to a `MaskSelect`.
class VISKORES_ALWAYS_EXPORT MaskSelectTemplate : public viskores::worklet::MaskSelect
{
public:
  /// Construct a `MaskSelect` object using an array that masks an output
  /// value with `0` and enables an output value with `1`.
  template <typename ArrayHandleType>
  MaskSelectTemplate(const ArrayHandleType& maskArray,
                     viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny())
    : MaskSelect(ThreadToOutputMapWrapper{ internal::MaskSelectBuild(maskArray, device) })
  {
  }
};

}
} // namespace viskores::worklet

#endif // viskores_worklet_MaskSelectTemplate_h
