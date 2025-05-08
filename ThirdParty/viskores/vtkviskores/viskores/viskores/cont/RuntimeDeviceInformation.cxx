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
#include <viskores/cont/RuntimeDeviceInformation.h>

#include <viskores/List.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/DeviceAdapterList.h>
#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/ErrorBadDevice.h>
#include <viskores/cont/Logging.h>

//Bring in each device adapters runtime class
#include <viskores/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>
#include <viskores/cont/kokkos/internal/DeviceAdapterRuntimeDetectorKokkos.h>
#include <viskores/cont/openmp/internal/DeviceAdapterRuntimeDetectorOpenMP.h>
#include <viskores/cont/serial/internal/DeviceAdapterRuntimeDetectorSerial.h>
#include <viskores/cont/tbb/internal/DeviceAdapterRuntimeDetectorTBB.h>

#include <cctype> //for tolower

namespace
{
class DeviceAdapterMemoryManagerInvalid
  : public viskores::cont::internal::DeviceAdapterMemoryManagerBase
{
public:
  VISKORES_CONT virtual ~DeviceAdapterMemoryManagerInvalid() override {}

  VISKORES_CONT virtual viskores::cont::internal::BufferInfo Allocate(
    viskores::BufferSizeType) const override
  {
    throw viskores::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }

  VISKORES_CONT virtual viskores::cont::DeviceAdapterId GetDevice() const override
  {
    return viskores::cont::DeviceAdapterTagUndefined{};
  }

  VISKORES_CONT virtual viskores::cont::internal::BufferInfo CopyHostToDevice(
    const viskores::cont::internal::BufferInfo&) const override
  {
    throw viskores::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }

  VISKORES_CONT virtual void CopyHostToDevice(
    const viskores::cont::internal::BufferInfo&,
    const viskores::cont::internal::BufferInfo&) const override
  {
    throw viskores::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }

  VISKORES_CONT virtual viskores::cont::internal::BufferInfo CopyDeviceToHost(
    const viskores::cont::internal::BufferInfo&) const override
  {
    throw viskores::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }

  VISKORES_CONT virtual void CopyDeviceToHost(
    const viskores::cont::internal::BufferInfo&,
    const viskores::cont::internal::BufferInfo&) const override
  {
    throw viskores::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }

  VISKORES_CONT virtual viskores::cont::internal::BufferInfo CopyDeviceToDevice(
    const viskores::cont::internal::BufferInfo&) const override
  {
    throw viskores::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }

  VISKORES_CONT virtual void CopyDeviceToDevice(
    const viskores::cont::internal::BufferInfo&,
    const viskores::cont::internal::BufferInfo&) const override
  {
    throw viskores::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }

  VISKORES_CONT virtual void DeleteRawPointer(void*) const override
  {
    throw viskores::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }
};

class RuntimeDeviceConfigurationInvalid final
  : public viskores::cont::internal::RuntimeDeviceConfigurationBase
{
public:
  VISKORES_CONT virtual ~RuntimeDeviceConfigurationInvalid() override final {}

  VISKORES_CONT virtual viskores::cont::DeviceAdapterId GetDevice() const override final
  {
    return viskores::cont::DeviceAdapterTagUndefined{};
  }

  VISKORES_CONT virtual viskores::cont::internal::RuntimeDeviceConfigReturnCode SetThreads(
    const viskores::Id&) override final
  {
    throw viskores::cont::ErrorBadDevice("Tried to set the number of threads on an invalid device");
  }

  VISKORES_CONT virtual viskores::cont::internal::RuntimeDeviceConfigReturnCode SetDeviceInstance(
    const viskores::Id&) override final
  {
    throw viskores::cont::ErrorBadDevice("Tried to set the device instance on an invalid device");
  }

  VISKORES_CONT virtual viskores::cont::internal::RuntimeDeviceConfigReturnCode GetThreads(
    viskores::Id&) const override final
  {
    throw viskores::cont::ErrorBadDevice("Tried to get the number of threads on an invalid device");
  }

  VISKORES_CONT virtual viskores::cont::internal::RuntimeDeviceConfigReturnCode GetDeviceInstance(
    viskores::Id&) const override final
  {
    throw viskores::cont::ErrorBadDevice("Tried to get the device instance on an invalid device");
  }

  VISKORES_CONT virtual viskores::cont::internal::RuntimeDeviceConfigReturnCode GetMaxThreads(
    viskores::Id&) const override final
  {
    throw viskores::cont::ErrorBadDevice(
      "Tried to get the max number of threads on an invalid device");
  }

  VISKORES_CONT virtual viskores::cont::internal::RuntimeDeviceConfigReturnCode GetMaxDevices(
    viskores::Id&) const override final
  {
    throw viskores::cont::ErrorBadDevice(
      "Tried to get the max number of devices on an invalid device");
  }
};


struct VISKORES_NEVER_EXPORT InitializeDeviceNames
{
  viskores::cont::DeviceAdapterNameType* Names;
  viskores::cont::DeviceAdapterNameType* LowerCaseNames;

  VISKORES_CONT
  InitializeDeviceNames(viskores::cont::DeviceAdapterNameType* names,
                        viskores::cont::DeviceAdapterNameType* lower)
    : Names(names)
    , LowerCaseNames(lower)
  {
    std::fill_n(this->Names, VISKORES_MAX_DEVICE_ADAPTER_ID, "InvalidDeviceId");
    std::fill_n(this->LowerCaseNames, VISKORES_MAX_DEVICE_ADAPTER_ID, "invaliddeviceid");
  }

  template <typename Device>
  VISKORES_CONT void operator()(Device device)
  {
    auto lowerCaseFunc = [](char c)
    { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };

    auto id = device.GetValue();

    if (id > 0 && id < VISKORES_MAX_DEVICE_ADAPTER_ID)
    {
      auto name = viskores::cont::DeviceAdapterTraits<Device>::GetName();
      this->Names[id] = name;
      std::transform(name.begin(), name.end(), name.begin(), lowerCaseFunc);
      this->LowerCaseNames[id] = name;
    }
  }
};

struct VISKORES_NEVER_EXPORT InitializeDeviceMemoryManagers
{
  std::unique_ptr<viskores::cont::internal::DeviceAdapterMemoryManagerBase>* Managers;

  VISKORES_CONT
  InitializeDeviceMemoryManagers(
    std::unique_ptr<viskores::cont::internal::DeviceAdapterMemoryManagerBase>* managers)
    : Managers(managers)
  {
  }

  template <typename Device>
  VISKORES_CONT void CreateManager(Device device, std::true_type)
  {
    auto id = device.GetValue();

    if (id > 0 && id < VISKORES_MAX_DEVICE_ADAPTER_ID)
    {
      this->Managers[id].reset(new viskores::cont::internal::DeviceAdapterMemoryManager<Device>);
    }
  }

  template <typename Device>
  VISKORES_CONT void CreateManager(Device, std::false_type)
  {
    // No manager for invalid devices.
  }

  template <typename Device>
  VISKORES_CONT void operator()(Device device)
  {
    this->CreateManager(device, std::integral_constant<bool, device.IsEnabled>{});
  }
};

struct VISKORES_NEVER_EXPORT InitializeRuntimeDeviceConfigurations
{
  std::unique_ptr<viskores::cont::internal::RuntimeDeviceConfigurationBase>* RuntimeConfigurations;
  viskores::cont::internal::RuntimeDeviceConfigurationOptions RuntimeConfigurationOptions;

  VISKORES_CONT
  InitializeRuntimeDeviceConfigurations(
    std::unique_ptr<viskores::cont::internal::RuntimeDeviceConfigurationBase>*
      runtimeConfigurations,
    const viskores::cont::internal::RuntimeDeviceConfigurationOptions& configOptions)
    : RuntimeConfigurations(runtimeConfigurations)
    , RuntimeConfigurationOptions(configOptions)
  {
    if (!configOptions.IsInitialized())
    {
      VISKORES_LOG_S(
        viskores::cont::LogLevel::Warn,
        "Initializing 'RuntimeDeviceConfigurations' with uninitialized configOptions. Did "
        "you call viskores::cont::Initialize?");
    }
  }

  template <typename Device>
  VISKORES_CONT void CreateRuntimeConfiguration(Device device,
                                                int& argc,
                                                char* argv[],
                                                std::true_type)
  {
    auto id = device.GetValue();

    if (id > 0 && id < VISKORES_MAX_DEVICE_ADAPTER_ID)
    {
      this->RuntimeConfigurations[id].reset(
        new viskores::cont::internal::RuntimeDeviceConfiguration<Device>);
      this->RuntimeConfigurations[id]->Initialize(RuntimeConfigurationOptions, argc, argv);
    }
  }

  template <typename Device>
  VISKORES_CONT void CreateRuntimeConfiguration(Device, int&, char**, std::false_type)
  {
    // No runtime configuration for invalid devices.
  }

  template <typename Device>
  VISKORES_CONT void operator()(Device device, int& argc, char* argv[])
  {
    this->CreateRuntimeConfiguration(
      device, argc, argv, std::integral_constant<bool, device.IsEnabled>{});
  }
};


struct VISKORES_NEVER_EXPORT RuntimeDeviceInformationFunctor
{
  bool Exists = false;
  template <typename DeviceAdapter>
  VISKORES_CONT void operator()(DeviceAdapter, viskores::cont::DeviceAdapterId device)
  {
    if (DeviceAdapter() == device)
    {
      this->Exists = viskores::cont::DeviceAdapterRuntimeDetector<DeviceAdapter>().Exists();
    }
  }
};

class RuntimeDeviceNames
{
public:
  static const viskores::cont::DeviceAdapterNameType& GetDeviceName(viskores::Int8 id)
  {
    return Instance().DeviceNames[id];
  }

  static const viskores::cont::DeviceAdapterNameType& GetLowerCaseDeviceName(viskores::Int8 id)
  {
    return Instance().LowerCaseDeviceNames[id];
  }

private:
  static const RuntimeDeviceNames& Instance()
  {
    static RuntimeDeviceNames instance;
    return instance;
  }

  RuntimeDeviceNames()
  {
    InitializeDeviceNames functor(DeviceNames, LowerCaseDeviceNames);
    viskores::ListForEach(functor, VISKORES_DEFAULT_DEVICE_ADAPTER_LIST());
  }

  friend struct InitializeDeviceNames;

  viskores::cont::DeviceAdapterNameType DeviceNames[VISKORES_MAX_DEVICE_ADAPTER_ID];
  viskores::cont::DeviceAdapterNameType LowerCaseDeviceNames[VISKORES_MAX_DEVICE_ADAPTER_ID];
};

class RuntimeDeviceMemoryManagers
{
public:
  static viskores::cont::internal::DeviceAdapterMemoryManagerBase& GetDeviceMemoryManager(
    viskores::cont::DeviceAdapterId device)
  {
    const auto id = device.GetValue();

    if (device.IsValueValid())
    {
      auto&& manager = Instance().DeviceMemoryManagers[id];
      if (manager)
      {
        return *manager.get();
      }
      else
      {
        return Instance().InvalidManager;
      }
    }
    else
    {
      return Instance().InvalidManager;
    }
  }

private:
  static RuntimeDeviceMemoryManagers& Instance()
  {
    static RuntimeDeviceMemoryManagers instance;
    return instance;
  }

  RuntimeDeviceMemoryManagers()
  {
    InitializeDeviceMemoryManagers functor(this->DeviceMemoryManagers);
    viskores::ListForEach(functor, VISKORES_DEFAULT_DEVICE_ADAPTER_LIST());
  }

  friend struct InitializeDeviceMemoryManagers;

  std::unique_ptr<viskores::cont::internal::DeviceAdapterMemoryManagerBase>
    DeviceMemoryManagers[VISKORES_MAX_DEVICE_ADAPTER_ID];
  DeviceAdapterMemoryManagerInvalid InvalidManager;
};

class RuntimeDeviceConfigurations
{
public:
  static viskores::cont::internal::RuntimeDeviceConfigurationBase& GetRuntimeDeviceConfiguration(
    viskores::cont::DeviceAdapterId device,
    const viskores::cont::internal::RuntimeDeviceConfigurationOptions& configOptions,
    int& argc,
    char* argv[])
  {
    const auto id = device.GetValue();
    if (device.IsValueValid())
    {
      auto&& runtimeConfiguration = Instance(configOptions, argc, argv).DeviceConfigurations[id];
      if (runtimeConfiguration)
      {
        return *runtimeConfiguration.get();
      }
      else
      {
        return Instance(configOptions, argc, argv).InvalidConfiguration;
      }
    }
    else
    {
      return Instance(configOptions, argc, argv).InvalidConfiguration;
    }
  }

private:
  static RuntimeDeviceConfigurations& Instance(
    const viskores::cont::internal::RuntimeDeviceConfigurationOptions& configOptions,
    int& argc,
    char* argv[])
  {
    static RuntimeDeviceConfigurations instance{ configOptions, argc, argv };
    return instance;
  }

  RuntimeDeviceConfigurations(
    const viskores::cont::internal::RuntimeDeviceConfigurationOptions& configOptions,
    int& argc,
    char* argv[])
  {
    InitializeRuntimeDeviceConfigurations functor(this->DeviceConfigurations, configOptions);
    viskores::ListForEach(functor, VISKORES_DEFAULT_DEVICE_ADAPTER_LIST(), argc, argv);
  }

  friend struct InitializeRuntimeDeviceConfigurations;

  std::unique_ptr<viskores::cont::internal::RuntimeDeviceConfigurationBase>
    DeviceConfigurations[VISKORES_MAX_DEVICE_ADAPTER_ID];
  RuntimeDeviceConfigurationInvalid InvalidConfiguration;
};
} // namespace

namespace viskores
{
namespace cont
{
namespace detail
{
}

VISKORES_CONT
DeviceAdapterNameType RuntimeDeviceInformation::GetName(DeviceAdapterId device) const
{
  const auto id = device.GetValue();

  if (device.IsValueValid())
  {
    return RuntimeDeviceNames::GetDeviceName(id);
  }
  else if (id == VISKORES_DEVICE_ADAPTER_UNDEFINED)
  {
    return viskores::cont::DeviceAdapterTraits<
      viskores::cont::DeviceAdapterTagUndefined>::GetName();
  }
  else if (id == VISKORES_DEVICE_ADAPTER_ANY)
  {
    return viskores::cont::DeviceAdapterTraits<viskores::cont::DeviceAdapterTagAny>::GetName();
  }

  // Deviceis invalid:
  return RuntimeDeviceNames::GetDeviceName(0);
}

VISKORES_CONT
DeviceAdapterId RuntimeDeviceInformation::GetId(DeviceAdapterNameType name) const
{
  // The GetDeviceAdapterId call is case-insensitive so transform the name to be lower case
  // as that is how we cache the case-insensitive version.
  auto lowerCaseFunc = [](char c)
  { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
  std::transform(name.begin(), name.end(), name.begin(), lowerCaseFunc);

  //lower-case the name here
  if (name == "any")
  {
    return viskores::cont::DeviceAdapterTagAny{};
  }
  else if (name == "undefined")
  {
    return viskores::cont::DeviceAdapterTagUndefined{};
  }

  for (viskores::Int8 id = 0; id < VISKORES_MAX_DEVICE_ADAPTER_ID; ++id)
  {
    if (name == RuntimeDeviceNames::GetLowerCaseDeviceName(id))
    {
      return viskores::cont::make_DeviceAdapterId(id);
    }
  }

  return viskores::cont::DeviceAdapterTagUndefined{};
}


VISKORES_CONT
bool RuntimeDeviceInformation::Exists(DeviceAdapterId id) const
{
  if (id == viskores::cont::DeviceAdapterTagAny{})
  {
    return true;
  }

  RuntimeDeviceInformationFunctor functor;
  viskores::ListForEach(functor, VISKORES_DEFAULT_DEVICE_ADAPTER_LIST(), id);
  return functor.Exists;
}

VISKORES_CONT viskores::cont::internal::DeviceAdapterMemoryManagerBase&
RuntimeDeviceInformation::GetMemoryManager(DeviceAdapterId device) const
{
  if (device.IsValueValid())
  {
    return RuntimeDeviceMemoryManagers::GetDeviceMemoryManager(device);
  }
  else
  {
    throw viskores::cont::ErrorBadValue(
      "Attempted to get a DeviceAdapterMemoryManager for an invalid device '" + device.GetName() +
      "'");
  }
}

VISKORES_CONT viskores::cont::internal::RuntimeDeviceConfigurationBase&
RuntimeDeviceInformation::GetRuntimeConfiguration(
  DeviceAdapterId device,
  const viskores::cont::internal::RuntimeDeviceConfigurationOptions& configOptions,
  int& argc,
  char* argv[]) const
{
  return RuntimeDeviceConfigurations::GetRuntimeDeviceConfiguration(
    device, configOptions, argc, argv);
}

VISKORES_CONT viskores::cont::internal::RuntimeDeviceConfigurationBase&
RuntimeDeviceInformation::GetRuntimeConfiguration(
  DeviceAdapterId device,
  const viskores::cont::internal::RuntimeDeviceConfigurationOptions& configOptions) const
{
  int placeholder;
  return this->GetRuntimeConfiguration(device, configOptions, placeholder, nullptr);
}

VISKORES_CONT viskores::cont::internal::RuntimeDeviceConfigurationBase&
RuntimeDeviceInformation::GetRuntimeConfiguration(DeviceAdapterId device) const
{
  viskores::cont::internal::RuntimeDeviceConfigurationOptions placeholder;
  return this->GetRuntimeConfiguration(device, placeholder);
}


} // namespace viskores::cont
} // namespace viskores
