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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/DeviceAdapterList.h>
#include <viskores/cont/Invoker.h>

#include <viskores/cont/internal/ArrayCopyUnknown.h>

#include <viskores/worklet/WorkletMapField.h>

namespace
{

// Use a worklet because device adapter copies often have an issue with casting the values from the
// `ArrayHandleRecomineVec` that comes from `UnknownArrayHandle::CastAndCallWithExtractedArray`.
struct CopyWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  template <typename InType, typename OutType>
  VISKORES_EXEC void operator()(const InType& in, OutType& out) const
  {
    out = in;
  }
};

struct UnknownCopyOnDevice
{
  bool Called = false;

  template <typename InType, typename OutType>
  void operator()(viskores::cont::DeviceAdapterId device,
                  const viskores::cont::ArrayHandleRecombineVec<InType>& in,
                  const viskores::cont::ArrayHandleRecombineVec<OutType>& out)
  {
    // Note: ArrayHandleRecombineVec returns the wrong value for IsOnDevice (always true).
    // This is one of the consequences of ArrayHandleRecombineVec breaking assumptions of
    // ArrayHandle. It does this by stuffing Buffer objects in another Buffer's meta data
    // rather than listing them explicitly (where they can be queried). We get around this
    // by pulling out one of the component arrays and querying that.
    if (!this->Called &&
        ((device == viskores::cont::DeviceAdapterTagAny{}) ||
         (in.GetComponentArray(0).IsOnDevice(device) &&
          viskores::cont::GetRuntimeDeviceTracker().CanRunOn(device))))
    {
      viskores::cont::Invoker invoke(device);
      invoke(CopyWorklet{}, in, out);
      this->Called = true;
    }
  }
};

struct UnknownCopyFunctor2
{
  template <typename OutType, typename InType>
  void operator()(const viskores::cont::ArrayHandleRecombineVec<OutType>& out,
                  const viskores::cont::ArrayHandleRecombineVec<InType>& in) const
  {
    UnknownCopyOnDevice doCopy;

    // Try to copy on a device that the data are already on.
    viskores::ListForEach(doCopy, VISKORES_DEFAULT_DEVICE_ADAPTER_LIST{}, in, out);

    // If it was not on any device, call one more time with any adapter to copy wherever.
    doCopy(viskores::cont::DeviceAdapterTagAny{}, in, out);
  }
};

struct UnknownCopyFunctor1
{
  template <typename InType>
  void operator()(const viskores::cont::ArrayHandleRecombineVec<InType>& in,
                  const viskores::cont::UnknownArrayHandle& out) const
  {
    out.Allocate(in.GetNumberOfValues());

    this->DoIt(in, out, typename std::is_same<viskores::FloatDefault, InType>::type{});
  }

  template <typename InType>
  void DoIt(const viskores::cont::ArrayHandleRecombineVec<InType>& in,
            const viskores::cont::UnknownArrayHandle& out,
            std::false_type) const
  {
    // Source is not float.
    if (out.IsBaseComponentType<InType>())
    {
      // Arrays have the same base component type. Copy directly.
      try
      {
        UnknownCopyFunctor2{}(out.ExtractArrayFromComponents<InType>(viskores::CopyFlag::Off), in);
      }
      catch (viskores::cont::Error& error)
      {
        throw viskores::cont::ErrorBadType(
          "Unable to copy to an array of type " + out.GetArrayTypeName() +
          " using anonymous methods. Try using viskores::cont::ArrayCopyDevice. "
          "(Original error: `" +
          error.GetMessage() + "')");
      }
    }
    else if (out.IsBaseComponentType<viskores::FloatDefault>())
    {
      // Can copy anything to default float.
      try
      {
        UnknownCopyFunctor2{}(
          out.ExtractArrayFromComponents<viskores::FloatDefault>(viskores::CopyFlag::Off), in);
      }
      catch (viskores::cont::Error& error)
      {
        throw viskores::cont::ErrorBadType(
          "Unable to copy to an array of type " + out.GetArrayTypeName() +
          " using anonymous methods. Try using viskores::cont::ArrayCopyDevice. "
          "(Original error: `" +
          error.GetMessage() + "')");
      }
    }
    else
    {
      // Arrays have different base types. To reduce the number of template paths from nxn to 3n,
      // copy first to a temp array of default float.
      viskores::cont::UnknownArrayHandle temp = out.NewInstanceFloatBasic();
      (*this)(in, temp);
      viskores::cont::ArrayCopy(temp, out);
    }
  }

  template <typename InType>
  void DoIt(const viskores::cont::ArrayHandleRecombineVec<InType>& in,
            const viskores::cont::UnknownArrayHandle& out,
            std::true_type) const
  {
    // Source array is FloatDefault. That should be copiable to anything.
    out.CastAndCallWithExtractedArray(UnknownCopyFunctor2{}, in);
  }
};

void ArrayCopySpecialCase(const viskores::cont::ArrayHandleIndex& source,
                          const viskores::cont::UnknownArrayHandle& destination)
{
  if (destination.CanConvert<viskores::cont::ArrayHandleIndex>())
  {
    // Unlikely, but we'll check.
    destination.AsArrayHandle<viskores::cont::ArrayHandleIndex>().DeepCopyFrom(source);
  }
  else if (destination.IsBaseComponentType<viskores::Id>())
  {
    destination.Allocate(source.GetNumberOfValues());
    auto dest = destination.ExtractComponent<viskores::Id>(0, viskores::CopyFlag::Off);
    viskores::cont::ArrayCopyDevice(source, dest);
  }
  else if (destination.IsBaseComponentType<viskores::IdComponent>())
  {
    destination.Allocate(source.GetNumberOfValues());
    auto dest = destination.ExtractComponent<viskores::IdComponent>(0, viskores::CopyFlag::Off);
    viskores::cont::ArrayCopyDevice(source, dest);
  }
  else if (destination.CanConvert<viskores::cont::ArrayHandle<viskores::FloatDefault>>())
  {
    viskores::cont::ArrayHandle<viskores::FloatDefault> dest;
    destination.AsArrayHandle(dest);
    viskores::cont::ArrayCopyDevice(source, dest);
  }
  else
  {
    // Initializing something that is probably not really an index. Rather than trace down every
    // unlikely possibility, just copy to float and then to the final array.
    viskores::cont::ArrayHandle<viskores::FloatDefault> dest;
    viskores::cont::ArrayCopyDevice(source, dest);
    viskores::cont::ArrayCopy(dest, destination);
  }
}

template <typename ArrayHandleType>
bool TryArrayCopySpecialCase(const viskores::cont::UnknownArrayHandle& source,
                             const viskores::cont::UnknownArrayHandle& destination)
{
  if (source.CanConvert<ArrayHandleType>())
  {
    ArrayCopySpecialCase(source.AsArrayHandle<ArrayHandleType>(), destination);
    return true;
  }
  else
  {
    return false;
  }
}

void DoUnknownArrayCopy(const viskores::cont::UnknownArrayHandle& source,
                        const viskores::cont::UnknownArrayHandle& destination)
{
  if (source.GetNumberOfValues() > 0)
  {
    // Try known special cases.
    if (TryArrayCopySpecialCase<viskores::cont::ArrayHandleIndex>(source, destination))
    {
      return;
    }

    source.CastAndCallWithExtractedArray(UnknownCopyFunctor1{}, destination);
  }
  else
  {
    destination.ReleaseResources();
  }
}

} // anonymous namespace

namespace viskores
{
namespace cont
{
namespace internal
{

void ArrayCopyUnknown(const viskores::cont::UnknownArrayHandle& source,
                      viskores::cont::UnknownArrayHandle& destination)
{
  if (!destination.IsValid())
  {
    destination = source.NewInstanceBasic();
  }

  DoUnknownArrayCopy(source, destination);
}

void ArrayCopyUnknown(const viskores::cont::UnknownArrayHandle& source,
                      const viskores::cont::UnknownArrayHandle& destination)
{
  if (!destination.IsValid())
  {
    throw viskores::cont::ErrorBadValue(
      "Attempty to copy to a constant UnknownArrayHandle with no valid array.");
  }

  DoUnknownArrayCopy(source, destination);
}

} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores
