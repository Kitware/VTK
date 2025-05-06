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

#include <viskores/cont/RuntimeDeviceInformation.h>

namespace viskores
{
namespace cont
{

DeviceAdapterNameType DeviceAdapterId::GetName() const
{
  viskores::cont::RuntimeDeviceInformation info;
  return info.GetName(*this);
}

DeviceAdapterId make_DeviceAdapterId(const DeviceAdapterNameType& name)
{
  viskores::cont::RuntimeDeviceInformation info;
  return info.GetId(name);
}
}
} // end namespace viskores::cont
