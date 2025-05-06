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
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/DeviceAdapterList.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/Timer.h>

#include <tuple>

namespace
{
template <typename Device>
using DeviceInvalid = std::integral_constant<bool, !Device::IsEnabled>;
using EnabledDeviceList =
  viskores::ListRemoveIf<viskores::cont::DeviceAdapterListCommon, DeviceInvalid>;

template <typename Device>
using DeviceTimerPtr = std::unique_ptr<viskores::cont::DeviceAdapterTimerImplementation<Device>>;

using EnabledTimerImpls = viskores::ListTransform<EnabledDeviceList, DeviceTimerPtr>;
using EnabledTimerImplTuple = viskores::ListApply<EnabledTimerImpls, std::tuple>;

// C++11 does not support get tuple element by type. C++14 does support that.
// Get the index of a type in tuple elements
template <class T, class Tuple>
struct Index;

template <class T, template <typename...> class Container, class... Types>
struct Index<T, Container<T, Types...>>
{
  static const std::size_t value = 0;
};

template <class T, class U, template <typename...> class Container, class... Types>
struct Index<T, Container<U, Types...>>
{
  static const std::size_t value = 1 + Index<T, Container<Types...>>::value;
};

template <typename Device>
VISKORES_CONT inline
  typename std::tuple_element<Index<Device, EnabledDeviceList>::value, EnabledTimerImplTuple>::type&
  GetUniqueTimerPtr(Device, EnabledTimerImplTuple& enabledTimers)
{
  return std::get<Index<Device, EnabledDeviceList>::value>(enabledTimers);
}

struct InitFunctor
{
  template <typename Device>
  VISKORES_CONT void operator()(Device, EnabledTimerImplTuple& timerImpls)
  {
    //We don't use the runtime device tracker to very initializtion support
    //so that the following use case is supported:
    //
    // GetRuntimeDeviceTracker().Disable( openMP );
    // viskores::cont::Timer timer; //tracks all active devices
    // GetRuntimeDeviceTracker().Enable( openMP );
    // timer.Start() //want to test openmp
    //
    // timer.GetElapsedTime()
    //
    // When `GetElapsedTime` is called we need to make sure that the OpenMP
    // device timer is safe to call. At the same time we still need to make
    // sure that we have the required runtime and not just compile time support
    // this is why we use `DeviceAdapterRuntimeDetector`
    bool haveRequiredRuntimeSupport =
      viskores::cont::DeviceAdapterRuntimeDetector<Device>{}.Exists();
    if (haveRequiredRuntimeSupport)
    {
      std::get<Index<Device, EnabledDeviceList>::value>(timerImpls)
        .reset(new viskores::cont::DeviceAdapterTimerImplementation<Device>());
    }
  }
};

struct ResetFunctor
{
  template <typename Device>
  VISKORES_CONT void operator()(Device device,
                                viskores::cont::DeviceAdapterId deviceToRunOn,
                                const viskores::cont::RuntimeDeviceTracker& tracker,
                                EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == viskores::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      GetUniqueTimerPtr(device, timerImpls)->Reset();
    }
  }
};

struct StartFunctor
{
  template <typename Device>
  VISKORES_CONT void operator()(Device device,
                                viskores::cont::DeviceAdapterId deviceToRunOn,
                                const viskores::cont::RuntimeDeviceTracker& tracker,
                                EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == viskores::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      GetUniqueTimerPtr(device, timerImpls)->Start();
    }
  }
};

struct StopFunctor
{
  template <typename Device>
  VISKORES_CONT void operator()(Device device,
                                viskores::cont::DeviceAdapterId deviceToRunOn,
                                const viskores::cont::RuntimeDeviceTracker& tracker,
                                EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == viskores::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      GetUniqueTimerPtr(device, timerImpls)->Stop();
    }
  }
};

struct StartedFunctor
{
  bool Value = true;

  template <typename Device>
  VISKORES_CONT void operator()(Device device,
                                viskores::cont::DeviceAdapterId deviceToRunOn,
                                const viskores::cont::RuntimeDeviceTracker& tracker,
                                EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == viskores::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      this->Value &= GetUniqueTimerPtr(device, timerImpls)->Started();
    }
  }
};

struct StoppedFunctor
{
  bool Value = true;

  template <typename Device>
  VISKORES_CONT void operator()(Device device,
                                viskores::cont::DeviceAdapterId deviceToRunOn,
                                const viskores::cont::RuntimeDeviceTracker& tracker,
                                EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == viskores::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      this->Value &= GetUniqueTimerPtr(device, timerImpls)->Stopped();
    }
  }
};

struct ReadyFunctor
{
  bool Value = true;

  template <typename Device>
  VISKORES_CONT void operator()(Device device,
                                viskores::cont::DeviceAdapterId deviceToRunOn,
                                const viskores::cont::RuntimeDeviceTracker& tracker,
                                EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == viskores::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      this->Value &= GetUniqueTimerPtr(device, timerImpls)->Ready();
    }
  }
};

struct ElapsedTimeFunctor
{
  viskores::Float64 ElapsedTime = 0.0;

  template <typename Device>
  VISKORES_CONT void operator()(Device device,
                                viskores::cont::DeviceAdapterId deviceToRunOn,
                                const viskores::cont::RuntimeDeviceTracker& tracker,
                                EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == viskores::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      this->ElapsedTime =
        viskores::Max(this->ElapsedTime, GetUniqueTimerPtr(device, timerImpls)->GetElapsedTime());
    }
  }
};
} // anonymous namespace


namespace viskores
{
namespace cont
{
namespace detail
{

struct EnabledDeviceTimerImpls
{
  EnabledDeviceTimerImpls()
  {
    viskores::ListForEach(InitFunctor(), EnabledDeviceList(), this->EnabledTimers);
  }
  ~EnabledDeviceTimerImpls() {}
  // A tuple of enabled timer implementations
  EnabledTimerImplTuple EnabledTimers;
};
}
}
} // namespace viskores::cont::detail


namespace viskores
{
namespace cont
{

Timer::Timer()
  : Device(viskores::cont::DeviceAdapterTagAny())
  , Internal(new detail::EnabledDeviceTimerImpls)
{
}

Timer::Timer(viskores::cont::DeviceAdapterId device)
  : Device(device)
  , Internal(new detail::EnabledDeviceTimerImpls)
{
  const auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  if (!tracker.CanRunOn(device))
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Error,
                   "Device '" << device.GetName()
                              << "' can not run on current Device."
                                 "Thus timer is not usable");
  }
}

Timer::~Timer() = default;

void Timer::Reset()
{
  const auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  viskores::ListForEach(
    ResetFunctor(), EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);
}

void Timer::Reset(viskores::cont::DeviceAdapterId device)
{
  const auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  if (!tracker.CanRunOn(device))
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Error,
                   "Device '" << device.GetName()
                              << "' can not run on current Device."
                                 "Thus timer is not usable");
  }

  this->Device = device;
  this->Reset();
}

void Timer::Start()
{
  const auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  viskores::ListForEach(
    StartFunctor(), EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);
}

void Timer::Stop()
{
  const auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  viskores::ListForEach(
    StopFunctor(), EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);
}

bool Timer::Started() const
{
  const auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  StartedFunctor functor;
  viskores::ListForEach(
    functor, EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);
  return functor.Value;
}

bool Timer::Stopped() const
{
  const auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  StoppedFunctor functor;
  viskores::ListForEach(
    functor, EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);
  return functor.Value;
}

bool Timer::Ready() const
{
  const auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  ReadyFunctor functor;
  viskores::ListForEach(
    functor, EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);
  return functor.Value;
}

viskores::Float64 Timer::GetElapsedTime() const
{
  //Throw an exception if a timer bound device now can't be used
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();

  ElapsedTimeFunctor functor;
  viskores::ListForEach(
    functor, EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);

  return functor.ElapsedTime;
}

void Timer::Synchronize() const
{
  viskores::cont::Algorithm::Synchronize(this->Device);
}

}
} // namespace viskores::cont
