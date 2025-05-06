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
#ifndef viskores_cont_serial_internal_RuntimeDeviceConfigurationSerial_h
#define viskores_cont_serial_internal_RuntimeDeviceConfigurationSerial_h

#include <viskores/cont/internal/RuntimeDeviceConfiguration.h>
#include <viskores/cont/serial/internal/DeviceAdapterTagSerial.h>

namespace viskores
{
namespace cont
{
namespace internal
{

template <>
class RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagSerial>
  : public viskores::cont::internal::RuntimeDeviceConfigurationBase
{
  VISKORES_CONT viskores::cont::DeviceAdapterId GetDevice() const final
  {
    return viskores::cont::DeviceAdapterTagSerial{};
  }
};
}
}
}

#endif //viskores_cont_serial_internal_RuntimeDeviceConfigurationSerial_h
