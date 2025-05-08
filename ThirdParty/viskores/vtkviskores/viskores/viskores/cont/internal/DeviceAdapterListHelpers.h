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
#ifndef viskores_cont_internal_DeviceAdapterListHelpers_h
#define viskores_cont_internal_DeviceAdapterListHelpers_h

#include <viskores/List.h>
#include <viskores/cont/ErrorBadDevice.h>
#include <viskores/cont/RuntimeDeviceTracker.h>

namespace viskores
{
namespace cont
{
namespace internal
{

//============================================================================
struct ExecuteIfValidDeviceTag
{

  template <typename DeviceAdapter>
  using EnableIfValid = std::enable_if<DeviceAdapter::IsEnabled>;

  template <typename DeviceAdapter>
  using EnableIfInvalid = std::enable_if<!DeviceAdapter::IsEnabled>;

  template <typename DeviceAdapter, typename Functor, typename... Args>
  typename EnableIfValid<DeviceAdapter>::type operator()(
    DeviceAdapter device,
    Functor&& f,
    const viskores::cont::RuntimeDeviceTracker& tracker,
    Args&&... args) const
  {
    if (tracker.CanRunOn(device))
    {
      f(device, std::forward<Args>(args)...);
    }
  }

  // do not generate code for invalid devices
  template <typename DeviceAdapter, typename... Args>
  typename EnableIfInvalid<DeviceAdapter>::type operator()(DeviceAdapter, Args&&...) const
  {
  }
};

/// Execute the given functor on each valid device in \c DeviceList.
///
template <typename DeviceList, typename Functor, typename... Args>
VISKORES_CONT void ForEachValidDevice(DeviceList devices, Functor&& functor, Args&&... args)
{
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  viskores::ListForEach(
    ExecuteIfValidDeviceTag{}, devices, functor, tracker, std::forward<Args>(args)...);
}
}
}
} // viskores::cont::internal

#endif // viskores_cont_internal_DeviceAdapterListHelpers_h
