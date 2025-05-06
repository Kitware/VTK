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
#ifndef viskores_cont_ErrorBadDevice_h
#define viskores_cont_ErrorBadDevice_h

#include <viskores/cont/Error.h>

#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace cont
{


VISKORES_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown when Viskores performs an operation that is not supported
/// on the current device.
///
class VISKORES_ALWAYS_EXPORT ErrorBadDevice : public Error
{
public:
  ErrorBadDevice(const std::string& message)
    : Error(message)
  {
  }
};

VISKORES_SILENCE_WEAK_VTABLE_WARNING_END

/// Throws an ErrorBadeDevice exception with the following message:
/// "Viskores was unable to transfer \c className to DeviceAdapter[id,name].
///  This is generally caused by asking for execution on a DeviceAdapter that
///  isn't compiled into Viskores. In the case of CUDA it can also be caused by accidentally
///  compiling source files as C++ files instead of CUDA."
//
VISKORES_CONT_EXPORT void throwFailedRuntimeDeviceTransfer(const std::string& className,
                                                           viskores::cont::DeviceAdapterId device);
}
} // namespace viskores::cont

#endif // viskores_cont_ErrorBadDevice_h
