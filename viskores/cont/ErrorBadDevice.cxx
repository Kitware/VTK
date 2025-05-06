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

#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/ErrorBadDevice.h>

#include <string>

namespace viskores
{
namespace cont
{

void throwFailedRuntimeDeviceTransfer(const std::string& className,
                                      viskores::cont::DeviceAdapterId deviceId)
{ //Should we support typeid() instead of className?
  const std::string msg = "Viskores was unable to transfer " + className +
    " to DeviceAdapter[id=" + std::to_string(deviceId.GetValue()) + ", name=" + deviceId.GetName() +
    "]. This is generally caused by asking for execution on a DeviceAdapter that "
    "isn't compiled into Viskores. In the case of CUDA it can also be caused by accidentally "
    "compiling source files as C++ files instead of CUDA.";
  throw viskores::cont::ErrorBadDevice(msg);
}
}
}
