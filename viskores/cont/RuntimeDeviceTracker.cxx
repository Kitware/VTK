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

#include <viskores/cont/RuntimeDeviceTracker.h>

#include <viskores/cont/ErrorBadValue.h>

#include <algorithm>
#include <array>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

namespace viskores
{
namespace cont
{

namespace detail
{

struct RuntimeDeviceTrackerInternals
{
  void ResetRuntimeAllowed() { this->RuntimeAllowed.fill(false); }

  void Reset() { this->ResetRuntimeAllowed(); }

  std::array<bool, VISKORES_MAX_DEVICE_ADAPTER_ID> RuntimeAllowed;
  bool ThreadFriendlyMemAlloc = false;
  std::function<bool()> AbortChecker;
};

}

VISKORES_CONT
RuntimeDeviceTracker::RuntimeDeviceTracker(detail::RuntimeDeviceTrackerInternals* details,
                                           bool reset)
  : Internals(details)
{
  if (reset)
  {
    this->Reset();
  }
}

VISKORES_CONT
RuntimeDeviceTracker::~RuntimeDeviceTracker() {}

VISKORES_CONT
void RuntimeDeviceTracker::CheckDevice(viskores::cont::DeviceAdapterId deviceId) const
{
  if (!deviceId.IsValueValid())
  {
    std::stringstream message;
    message << "Device '" << deviceId.GetName() << "' has invalid ID of "
            << (int)deviceId.GetValue();
    throw viskores::cont::ErrorBadValue(message.str());
  }
}

VISKORES_CONT
bool RuntimeDeviceTracker::CanRunOn(viskores::cont::DeviceAdapterId deviceId) const
{
  if (deviceId == viskores::cont::DeviceAdapterTagAny{})
  { //If at least a single device is enabled, than any device is enabled
    for (viskores::Int8 i = 1; i < VISKORES_MAX_DEVICE_ADAPTER_ID; ++i)
    {
      if (this->Internals->RuntimeAllowed[static_cast<std::size_t>(i)])
      {
        return true;
      }
    }
    return false;
  }
  else
  {
    this->CheckDevice(deviceId);
    return this->Internals->RuntimeAllowed[static_cast<std::size_t>(deviceId.GetValue())];
  }
}

VISKORES_CONT
bool RuntimeDeviceTracker::GetThreadFriendlyMemAlloc() const
{
  return this->Internals->ThreadFriendlyMemAlloc;
}

VISKORES_CONT
void RuntimeDeviceTracker::SetDeviceState(viskores::cont::DeviceAdapterId deviceId, bool state)
{
  this->CheckDevice(deviceId);

  this->Internals->RuntimeAllowed[static_cast<std::size_t>(deviceId.GetValue())] = state;
}

VISKORES_CONT
void RuntimeDeviceTracker::SetThreadFriendlyMemAlloc(bool state)
{
  this->Internals->ThreadFriendlyMemAlloc = state;
}

VISKORES_CONT void RuntimeDeviceTracker::ResetDevice(viskores::cont::DeviceAdapterId deviceId)
{
  if (deviceId == viskores::cont::DeviceAdapterTagAny{})
  {
    this->Reset();
  }
  else
  {
    viskores::cont::RuntimeDeviceInformation runtimeDevice;
    this->SetDeviceState(deviceId, runtimeDevice.Exists(deviceId));
    this->LogEnabledDevices();
  }
}


VISKORES_CONT
void RuntimeDeviceTracker::Reset()
{
  this->Internals->Reset();

  // We use this instead of calling CheckDevice/SetDeviceState so that
  // when we use logging we get better messages stating we are reseting
  // the devices.
  viskores::cont::RuntimeDeviceInformation runtimeDevice;
  for (viskores::Int8 i = 1; i < VISKORES_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    viskores::cont::DeviceAdapterId device = viskores::cont::make_DeviceAdapterId(i);
    if (device.IsValueValid())
    {
      const bool state = runtimeDevice.Exists(device);
      this->Internals->RuntimeAllowed[static_cast<std::size_t>(device.GetValue())] = state;
    }
  }
  this->LogEnabledDevices();
}

VISKORES_CONT void RuntimeDeviceTracker::DisableDevice(viskores::cont::DeviceAdapterId deviceId)
{
  if (deviceId == viskores::cont::DeviceAdapterTagAny{})
  {
    this->Internals->ResetRuntimeAllowed();
  }
  else
  {
    this->SetDeviceState(deviceId, false);
  }
  this->LogEnabledDevices();
}

VISKORES_CONT
void RuntimeDeviceTracker::ForceDevice(DeviceAdapterId deviceId)
{
  if (deviceId == viskores::cont::DeviceAdapterTagAny{})
  {
    this->Reset();
  }
  else
  {
    this->CheckDevice(deviceId);
    viskores::cont::RuntimeDeviceInformation runtimeDevice;
    const bool runtimeExists = runtimeDevice.Exists(deviceId);
    if (!runtimeExists)
    {
      std::stringstream message;
      message << "Cannot force to device '" << deviceId.GetName()
              << "' because that device is not available on this system";
      throw viskores::cont::ErrorBadValue(message.str());
    }

    this->Internals->ResetRuntimeAllowed();
    this->Internals->RuntimeAllowed[static_cast<std::size_t>(deviceId.GetValue())] = runtimeExists;
    this->LogEnabledDevices();
  }
}

VISKORES_CONT void RuntimeDeviceTracker::CopyStateFrom(
  const viskores::cont::RuntimeDeviceTracker& tracker)
{
  *(this->Internals) = *tracker.Internals;
}

VISKORES_CONT
void RuntimeDeviceTracker::SetAbortChecker(const std::function<bool()>& func)
{
  this->Internals->AbortChecker = func;
}

VISKORES_CONT
bool RuntimeDeviceTracker::CheckForAbortRequest() const
{
  if (this->Internals->AbortChecker)
  {
    return this->Internals->AbortChecker();
  }
  return false;
}

VISKORES_CONT
void RuntimeDeviceTracker::ClearAbortChecker()
{
  this->Internals->AbortChecker = nullptr;
}

VISKORES_CONT
void RuntimeDeviceTracker::PrintSummary(std::ostream& out) const
{
  for (viskores::Int8 i = 1; i < VISKORES_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    auto dev = viskores::cont::make_DeviceAdapterId(i);
    out << " - Device " << static_cast<viskores::Int32>(i) << " (" << dev.GetName()
        << "): Enabled=" << this->CanRunOn(dev) << "\n";
  }
}

VISKORES_CONT
void RuntimeDeviceTracker::LogEnabledDevices() const
{
  std::stringstream message;
  message << "Enabled devices:";
  bool atLeastOneDeviceEnabled = false;
  for (viskores::Int8 deviceIndex = 1; deviceIndex < VISKORES_MAX_DEVICE_ADAPTER_ID; ++deviceIndex)
  {
    viskores::cont::DeviceAdapterId device = viskores::cont::make_DeviceAdapterId(deviceIndex);
    if (this->CanRunOn(device))
    {
      message << " " << device.GetName();
      atLeastOneDeviceEnabled = true;
    }
  }
  if (!atLeastOneDeviceEnabled)
  {
    message << " NONE!";
  }
  VISKORES_LOG_S(viskores::cont::LogLevel::DevicesEnabled, message.str());
}

VISKORES_CONT
ScopedRuntimeDeviceTracker::ScopedRuntimeDeviceTracker(
  const viskores::cont::RuntimeDeviceTracker& tracker)
  : RuntimeDeviceTracker(tracker.Internals, false)
  , SavedState(new detail::RuntimeDeviceTrackerInternals(*this->Internals))
{
  VISKORES_LOG_S(viskores::cont::LogLevel::DevicesEnabled, "Entering scoped runtime region");
}

VISKORES_CONT
ScopedRuntimeDeviceTracker::ScopedRuntimeDeviceTracker(
  viskores::cont::DeviceAdapterId device,
  RuntimeDeviceTrackerMode mode,
  const viskores::cont::RuntimeDeviceTracker& tracker)
  : ScopedRuntimeDeviceTracker(tracker)
{
  if (mode == RuntimeDeviceTrackerMode::Force)
  {
    this->ForceDevice(device);
  }
  else if (mode == RuntimeDeviceTrackerMode::Enable)
  {
    this->ResetDevice(device);
  }
  else if (mode == RuntimeDeviceTrackerMode::Disable)
  {
    this->DisableDevice(device);
  }
}

VISKORES_CONT ScopedRuntimeDeviceTracker::ScopedRuntimeDeviceTracker(
  const std::function<bool()>& abortChecker,
  const viskores::cont::RuntimeDeviceTracker& tracker)
  : ScopedRuntimeDeviceTracker(tracker)
{
  this->SetAbortChecker(abortChecker);
}

VISKORES_CONT
ScopedRuntimeDeviceTracker::~ScopedRuntimeDeviceTracker()
{
  VISKORES_LOG_S(viskores::cont::LogLevel::DevicesEnabled, "Leaving scoped runtime region");
  *(this->Internals) = *this->SavedState;

  this->LogEnabledDevices();
}

VISKORES_CONT
viskores::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker()
{
  using SharedTracker = std::shared_ptr<viskores::cont::RuntimeDeviceTracker>;
  static thread_local viskores::cont::detail::RuntimeDeviceTrackerInternals details;
  static thread_local SharedTracker runtimeDeviceTracker;
  static std::weak_ptr<viskores::cont::RuntimeDeviceTracker> defaultRuntimeDeviceTracker;

  if (runtimeDeviceTracker)
  {
    return *runtimeDeviceTracker;
  }

  // The RuntimeDeviceTracker for this thread has not been created. Create a new one.
  runtimeDeviceTracker = SharedTracker(new viskores::cont::RuntimeDeviceTracker(&details, true));

  // Get the default details, which are a global variable, with thread safety
  static std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);

  SharedTracker defaultTracker = defaultRuntimeDeviceTracker.lock();

  if (defaultTracker)
  {
    // We already have a default tracker, so copy the state from there. We don't need to
    // keep our mutex locked because we already have a safe handle to the defaultTracker.
    lock.unlock();
    runtimeDeviceTracker->CopyStateFrom(*defaultTracker);
  }
  else
  {
    // There is no default tracker yet. It has never been created (or it was on a thread
    // that got deleted). Use the current thread's details as the default.
    defaultRuntimeDeviceTracker = runtimeDeviceTracker;
  }

  return *runtimeDeviceTracker;
}

}
} // namespace viskores::cont
