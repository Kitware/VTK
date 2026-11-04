// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkmDeviceSelection.h"

#include "vtkObjectFactory.h"
#include "vtkmlib/vtkmInitializer.h"

#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/Error.h>
#include <viskores/cont/RuntimeDeviceInformation.h>
#include <viskores/cont/RuntimeDeviceTracker.h>

#include <string>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkmDeviceSelection);

namespace
{
//------------------------------------------------------------------------------
// The device Viskores calls @a name, or an invalid id after saying why not.
//
// Viskores is brought up first: on Kokkos the tracker is meaningless until
// it is, and nothing else may have run a filter yet.
viskores::cont::DeviceAdapterId DeviceNamed(const char* name)
{
  vtkmInitializer initialize;
  (void)initialize;
  const auto device = viskores::cont::make_DeviceAdapterId(name ? name : "");
  if (!device.IsValueValid())
  {
    vtkGenericWarningMacro("Unknown Viskores device \"" << (name ? name : "") << "\".");
  }
  return device;
}
}

//------------------------------------------------------------------------------
vtkmDeviceSelection::vtkmDeviceSelection() = default;

//------------------------------------------------------------------------------
vtkmDeviceSelection::~vtkmDeviceSelection() = default;

//------------------------------------------------------------------------------
void vtkmDeviceSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AvailableDevices: " << vtkmDeviceSelection::GetAvailableDevices() << "\n";
}

//------------------------------------------------------------------------------
bool vtkmDeviceSelection::ForceDevice(const char* name)
{
  const auto device = DeviceNamed(name);
  if (!device.IsValueValid())
  {
    return false;
  }
  // Viskores throws rather than force a device that cannot run, and an
  // exception must not cross into the wrappers.
  try
  {
    viskores::cont::GetRuntimeDeviceTracker().ForceDevice(device);
  }
  catch (const viskores::cont::Error& e)
  {
    vtkGenericWarningMacro(
      "Cannot run on Viskores device \"" << device.GetName() << "\": " << e.GetMessage());
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkmDeviceSelection::DisableDevice(const char* name)
{
  const auto device = DeviceNamed(name);
  if (!device.IsValueValid())
  {
    return false;
  }
  viskores::cont::GetRuntimeDeviceTracker().DisableDevice(device);
  return true;
}

//------------------------------------------------------------------------------
bool vtkmDeviceSelection::EnableDevice(const char* name)
{
  const auto device = DeviceNamed(name);
  if (!device.IsValueValid())
  {
    return false;
  }
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  tracker.ResetDevice(device);
  // Resetting a device this build or machine cannot run leaves it unavailable.
  if (!tracker.CanRunOn(device))
  {
    vtkGenericWarningMacro("Cannot run on Viskores device \"" << device.GetName() << "\".");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkmDeviceSelection::Reset()
{
  vtkmInitializer initialize;
  (void)initialize;
  viskores::cont::GetRuntimeDeviceTracker().Reset();
}

//------------------------------------------------------------------------------
const char* vtkmDeviceSelection::GetAvailableDevices()
{
  vtkmInitializer initialize;
  (void)initialize;

  static thread_local std::string result;
  result.clear();
  const auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  for (viskores::Int8 id = 1; id < VISKORES_MAX_DEVICE_ADAPTER_ID; ++id)
  {
    const auto device = viskores::cont::make_DeviceAdapterId(id);
    if (tracker.CanRunOn(device))
    {
      if (!result.empty())
      {
        result += ' ';
      }
      result += device.GetName();
    }
  }
  return result.c_str();
}

VTK_ABI_NAMESPACE_END
