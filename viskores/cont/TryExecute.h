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
#ifndef viskores_cont_TryExecute_h
#define viskores_cont_TryExecute_h

#include <viskores/cont/DeviceAdapterList.h>
#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/ErrorUserAbort.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/RuntimeDeviceTracker.h>


namespace viskores
{
namespace cont
{

namespace detail
{

VISKORES_CONT_EXPORT void HandleTryExecuteException(viskores::cont::DeviceAdapterId,
                                                    viskores::cont::RuntimeDeviceTracker&,
                                                    const std::string& functorName);

template <typename DeviceTag, typename Functor, typename... Args>
inline bool TryExecuteIfValid(std::true_type,
                              DeviceTag tag,
                              Functor&& f,
                              viskores::cont::DeviceAdapterId devId,
                              viskores::cont::RuntimeDeviceTracker& tracker,
                              Args&&... args)
{
  if ((tag == devId || devId == DeviceAdapterTagAny()) && tracker.CanRunOn(tag))
  {
    try
    {
      if (tracker.CheckForAbortRequest())
      {
        throw viskores::cont::ErrorUserAbort{};
      }

      return f(tag, std::forward<Args>(args)...);
    }
    catch (...)
    {
      detail::HandleTryExecuteException(tag, tracker, viskores::cont::TypeToString<Functor>());
    }
  }

  // If we are here, then the functor was either never run or failed.
  return false;
}

template <typename DeviceTag, typename Functor, typename... Args>
inline bool TryExecuteIfValid(std::false_type,
                              DeviceTag,
                              Functor&&,
                              viskores::cont::DeviceAdapterId,
                              viskores::cont::RuntimeDeviceTracker&,
                              Args&&...)
{
  return false;
}

struct TryExecuteWrapper
{
  template <typename DeviceTag, typename Functor, typename... Args>
  inline void operator()(DeviceTag tag,
                         Functor&& f,
                         viskores::cont::DeviceAdapterId devId,
                         viskores::cont::RuntimeDeviceTracker& tracker,
                         bool& ran,
                         Args&&... args) const
  {
    if (!ran)
    {
      ran = TryExecuteIfValid(std::integral_constant<bool, DeviceTag::IsEnabled>(),
                              tag,
                              std::forward<Functor>(f),
                              devId,
                              std::forward<decltype(tracker)>(tracker),
                              std::forward<Args>(args)...);
    }
  }
};

template <typename Functor, typename DeviceList, typename... Args>
inline bool TryExecuteImpl(viskores::cont::DeviceAdapterId devId,
                           Functor&& functor,
                           std::true_type,
                           DeviceList list,
                           Args&&... args)
{
  bool success = false;
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  TryExecuteWrapper task;
  viskores::ListForEach(task,
                        list,
                        std::forward<Functor>(functor),
                        devId,
                        tracker,
                        success,
                        std::forward<Args>(args)...);
  return success;
}

template <typename Functor, typename... Args>
inline bool TryExecuteImpl(viskores::cont::DeviceAdapterId devId,
                           Functor&& functor,
                           std::false_type,
                           Args&&... args)
{
  bool success = false;
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  TryExecuteWrapper task;
  viskores::ListForEach(task,
                        VISKORES_DEFAULT_DEVICE_ADAPTER_LIST(),
                        std::forward<Functor>(functor),
                        devId,
                        tracker,
                        success,
                        std::forward<Args>(args)...);
  return success;
}
} // namespace detail

///@{
/// \brief Try to execute a functor on a specific device selected at runtime.
///
/// This function takes a functor and a \c DeviceAdapterId which represents a
/// specific device to attempt to run on at runtime. It also optionally accepts
/// a set of devices to compile support for.
///
/// It then iterates over the set of devices finding which one matches the provided
/// adapter Id and is also enabled in the runtime. The function will return true
/// only if the device adapter was valid, and the task was successfully run.
///
/// The TryExecuteOnDevice is also able to perfectly forward arbitrary arguments onto the functor.
/// These arguments must be placed after the optional device adapter list and will passed to
/// the functor in the same order as listed.
///
/// The functor must implement the function call operator ( \c operator() ) with a return type of
/// \c bool and that is \c true if the execution succeeds, \c false if it fails. If an exception
/// is thrown from the functor, then the execution is assumed to have failed. The functor call
/// operator must also take at least one argument being the required \c DeviceAdapterTag to use.
///
/// \code{.cpp}
/// struct TryCallExample
/// {
///   template<typename DeviceList>
///   bool operator()(DeviceList tags, int) const
///   {
///     return true;
///   }
/// };
///
///
/// // Execute only on the device which corresponds to devId
/// // Will not execute all if devId is
/// viskores::cont::TryExecuteOnDevice(devId, TryCallExample(), int{42});
///
/// \endcode
///
/// This function returns \c true if the functor succeeded on a device,
/// \c false otherwise.
///
/// If no device list is specified, then \c VISKORES_DEFAULT_DEVICE_ADAPTER_LIST
/// is used.
///
template <typename Functor>
VISKORES_CONT bool TryExecuteOnDevice(viskores::cont::DeviceAdapterId devId, Functor&& functor)
{
  //we haven't been passed either a runtime tracker or a device list
  return detail::TryExecuteImpl(devId, std::forward<Functor>(functor), std::false_type{});
}
template <typename Functor, typename Arg1, typename... Args>
VISKORES_CONT bool TryExecuteOnDevice(viskores::cont::DeviceAdapterId devId,
                                      Functor&& functor,
                                      Arg1&& arg1,
                                      Args&&... args)
{
  //determine if we are being passed a device adapter or runtime tracker as our argument
  using is_deviceAdapter = viskores::internal::IsList<Arg1>;

  return detail::TryExecuteImpl(devId,
                                std::forward<Functor>(functor),
                                is_deviceAdapter{},
                                std::forward<Arg1>(arg1),
                                std::forward<Args>(args)...);
}

//@} //block doxygen all TryExecuteOnDevice functions

///@{
/// \brief Try to execute a functor on a set of devices until one succeeds.
///
/// This function takes a functor and optionally a set of devices to compile support.
/// It then tries to run the functor for each device (in the order given in the list) until the
/// execution succeeds.
///
/// The TryExecute is also able to perfectly forward arbitrary arguments onto the functor.
/// These arguments must be placed after the optional device adapter list and will passed
///  to the functor in the same order as listed.
///
/// The functor must implement the function call operator ( \c operator() ) with a return type of
/// \c bool and that is \c true if the execution succeeds, \c false if it fails. If an exception
/// is thrown from the functor, then the execution is assumed to have failed. The functor call
/// operator must also take at least one argument being the required \c DeviceAdapterTag to use.
///
/// \code{.cpp}
/// struct TryCallExample
/// {
///   template<typename DeviceList>
///   bool operator()(DeviceList tags, int) const
///   {
///     return true;
///   }
/// };
///
///
/// // Executing without a deviceId, or device list
/// viskores::cont::TryExecute(TryCallExample(), int{42});
///
/// // Executing with a device list
/// using DeviceList = viskores::List<viskores::cont::DeviceAdapterTagSerial>;
/// viskores::cont::TryExecute(TryCallExample(), DeviceList(), int{42});
///
/// \endcode
///
/// This function returns \c true if the functor succeeded on a device,
/// \c false otherwise.
///
/// If no device list is specified, then \c VISKORES_DEFAULT_DEVICE_ADAPTER_LIST
/// is used.
///
template <typename Functor, typename... Args>
VISKORES_CONT bool TryExecute(Functor&& functor, Args&&... args)
{
  return TryExecuteOnDevice(viskores::cont::DeviceAdapterTagAny(),
                            std::forward<Functor>(functor),
                            std::forward<Args>(args)...);
}


//@} //block doxygen all TryExecute functions
}
} // namespace viskores::cont

#endif //viskores_cont_TryExecute_h
