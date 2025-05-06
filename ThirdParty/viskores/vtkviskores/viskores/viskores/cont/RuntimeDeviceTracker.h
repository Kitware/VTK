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
#ifndef viskores_cont_RuntimeDeviceTracker_h
#define viskores_cont_RuntimeDeviceTracker_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/ErrorBadAllocation.h>
#include <viskores/cont/ErrorBadDevice.h>
#include <viskores/cont/RuntimeDeviceInformation.h>

#include <functional>
#include <memory>

namespace viskores
{
namespace cont
{
namespace detail
{

struct RuntimeDeviceTrackerInternals;
}
class ScopedRuntimeDeviceTracker;

/// RuntimeDeviceTracker is the central location for determining
/// which device adapter will be active for algorithm execution.
/// Many features in Viskores will attempt to run algorithms on the "best
/// available device." This generally is determined at runtime as some
/// backends require specific hardware, or failures in one device are
/// recorded and that device is disabled.
///
/// While viskores::cont::RunimeDeviceInformation reports on the existence
/// of a device being supported, this tracks on a per-thread basis
/// when worklets fail, why the fail, and will update the list
/// of valid runtime devices based on that information.
///
///
class VISKORES_CONT_EXPORT RuntimeDeviceTracker
{
  friend VISKORES_CONT_EXPORT viskores::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker();

public:
  VISKORES_CONT
  ~RuntimeDeviceTracker();

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  VISKORES_CONT bool CanRunOn(DeviceAdapterId deviceId) const;

  /// Report a failure to allocate memory on a device, this will flag the
  /// device as being unusable for all future invocations.
  ///
  VISKORES_CONT void ReportAllocationFailure(viskores::cont::DeviceAdapterId deviceId,
                                             const viskores::cont::ErrorBadAllocation&)
  {
    this->SetDeviceState(deviceId, false);
  }


  /// Report a ErrorBadDevice failure and flag the device as unusable.
  VISKORES_CONT void ReportBadDeviceFailure(viskores::cont::DeviceAdapterId deviceId,
                                            const viskores::cont::ErrorBadDevice&)
  {
    this->SetDeviceState(deviceId, false);
  }

  /// Reset the tracker for the given device. This will discard any updates
  /// caused by reported failures. Passing DeviceAdapterTagAny to this will
  /// reset all devices (same as `Reset()`).
  ///
  VISKORES_CONT void ResetDevice(viskores::cont::DeviceAdapterId deviceId);

  /// Reset the tracker to its default state for default devices.
  /// Will discard any updates caused by reported failures.
  ///
  VISKORES_CONT
  void Reset();

  /// @brief Disable the given device.
  ///
  /// The main intention of `RuntimeDeviceTracker` is to keep track of what
  /// devices are working for Viskores. However, it can also be used to turn
  /// devices on and off. Use this method to disable (turn off) a given device.
  /// Use `ResetDevice()` to turn the device back on (if it is supported).
  ///
  /// Passing DeviceAdapterTagAny to this will disable all devices.
  ///
  VISKORES_CONT void DisableDevice(DeviceAdapterId deviceId);

  /// \brief Disable all devices except the specified one.
  ///
  /// The main intention of `RuntimeDeviceTracker` is to keep track of what
  /// devices are working for Viskores. However, it can also be used to turn
  /// devices on and off. Use this method to disable all devices except one
  /// to effectively force Viskores to use that device. Either pass the
  /// DeviceAdapterTagAny to this function or call `Reset()` to restore
  /// all devices to their default state.
  ///
  /// This method will throw a `viskores::cont::ErrorBadValue` if the given device
  /// does not exist on the system.
  ///
  VISKORES_CONT void ForceDevice(DeviceAdapterId deviceId);

  /// @brief Get/Set use of thread-friendly memory allocation for a device.
  ///
  ///
  VISKORES_CONT bool GetThreadFriendlyMemAlloc() const;
  /// @copydoc GetThreadFriendlyMemAlloc
  VISKORES_CONT void SetThreadFriendlyMemAlloc(bool state);

  /// @brief Copies the state from the given device.
  ///
  /// This is a convenient way to allow the `RuntimeDeviceTracker` on one thread
  /// copy the behavior from another thread.
  ///
  VISKORES_CONT void CopyStateFrom(const viskores::cont::RuntimeDeviceTracker& tracker);

  /// @brief Set/Clear the abort checker functor.
  ///
  /// If set the abort checker functor is called by `viskores::cont::TryExecute()`
  /// before scheduling a task on a device from the associated the thread. If
  /// the functor returns `true`, an exception is thrown.
  VISKORES_CONT void SetAbortChecker(const std::function<bool()>& func);
  /// @copydoc SetAbortChecker
  VISKORES_CONT void ClearAbortChecker();

  VISKORES_CONT bool CheckForAbortRequest() const;

  /// @brief Produce a human-readable report on the state of the runtime device tracker.
  VISKORES_CONT void PrintSummary(std::ostream& out) const;

private:
  friend class ScopedRuntimeDeviceTracker;

  detail::RuntimeDeviceTrackerInternals* Internals;

  VISKORES_CONT
  RuntimeDeviceTracker(detail::RuntimeDeviceTrackerInternals* details, bool reset);

  VISKORES_CONT
  RuntimeDeviceTracker(const RuntimeDeviceTracker&) = delete;

  VISKORES_CONT
  RuntimeDeviceTracker& operator=(const RuntimeDeviceTracker&) = delete;

  VISKORES_CONT
  void CheckDevice(viskores::cont::DeviceAdapterId deviceId) const;

  VISKORES_CONT
  void SetDeviceState(viskores::cont::DeviceAdapterId deviceId, bool state);

  VISKORES_CONT
  void LogEnabledDevices() const;
};

///----------------------------------------------------------------------------
/// \brief Get the \c RuntimeDeviceTracker for the current thread.
///
/// Many features in Viskores will attempt to run algorithms on the "best
/// available device." This often is determined at runtime as failures in
/// one device are recorded and that device is disabled. To prevent having
/// to check over and over again, Viskores uses per thread runtime device tracker
/// so that these choices are marked and shared.
///
VISKORES_CONT_EXPORT
VISKORES_CONT
viskores::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker();

/// @brief Identifier used to specify whether to enable or disable a particular device.
enum struct RuntimeDeviceTrackerMode
{
  // Documentation is below (for better layout in generated documents).
  Force,
  Enable,
  Disable
};

/// @var RuntimeDeviceTrackerMode Force
/// @brief Replaces the current list of devices to try with the device specified.
///
/// This has the effect of forcing Viskores to use the provided device.
/// This is the default behavior for `viskores::cont::ScopedRuntimeDeviceTracker`.

/// @var RuntimeDeviceTrackerMode Enable
/// @brief Adds the provided device adapter to the list of devices to try.

/// @var RuntimeDeviceTrackerMode Disable
/// @brief Removes the provided device adapter from the list of devices to try.

//----------------------------------------------------------------------------
/// A class to create a scoped runtime device tracker object. This object captures the state
/// of the per-thread device tracker and will revert any changes applied
/// during its lifetime on destruction.
///
class VISKORES_CONT_EXPORT ScopedRuntimeDeviceTracker : public viskores::cont::RuntimeDeviceTracker
{
public:
  /// Construct a ScopedRuntimeDeviceTracker associated with the thread,
  /// associated with the provided tracker (defaults to current thread's tracker).
  ///
  /// Any modifications to the ScopedRuntimeDeviceTracker will effect what
  /// ever thread the \c tracker is associated with, which might not be
  /// the thread on which the ScopedRuntimeDeviceTracker was constructed.
  ///
  /// Constructors are not thread safe
  /// @{
  ///
  VISKORES_CONT ScopedRuntimeDeviceTracker(
    const viskores::cont::RuntimeDeviceTracker& tracker = GetRuntimeDeviceTracker());

  /// Use this constructor to modify the state of the device adapters associated with
  /// the provided tracker. Use \p mode with \p device as follows:
  ///
  /// 'Force' (default)
  ///   - Force-Enable the provided single device adapter
  ///   - Force-Enable all device adapters when using viskores::cont::DeviceAdaterTagAny
  /// 'Enable'
  ///   - Enable the provided single device adapter if it was previously disabled
  ///   - Enable all device adapters that are currently disabled when using
  ///     viskores::cont::DeviceAdaterTagAny
  /// 'Disable'
  ///   - Disable the provided single device adapter
  ///   - Disable all device adapters when using viskores::cont::DeviceAdaterTagAny
  ///
  VISKORES_CONT ScopedRuntimeDeviceTracker(
    viskores::cont::DeviceAdapterId device,
    RuntimeDeviceTrackerMode mode = RuntimeDeviceTrackerMode::Force,
    const viskores::cont::RuntimeDeviceTracker& tracker = GetRuntimeDeviceTracker());

  /// Use this constructor to set the abort checker functor for the provided tracker.
  ///
  VISKORES_CONT ScopedRuntimeDeviceTracker(
    const std::function<bool()>& abortChecker,
    const viskores::cont::RuntimeDeviceTracker& tracker = GetRuntimeDeviceTracker());

  /// Destructor is not thread safe
  VISKORES_CONT ~ScopedRuntimeDeviceTracker();

private:
  std::unique_ptr<detail::RuntimeDeviceTrackerInternals> SavedState;
};

}
} // namespace viskores::cont

#endif //viskores_cont_RuntimeDeviceTracker_h
