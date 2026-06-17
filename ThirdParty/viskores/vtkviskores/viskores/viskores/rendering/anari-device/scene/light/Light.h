//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#pragma once

#include "Object.h"

namespace viskores_device
{

struct Light : public Object
{
  Light(ViskoresDeviceGlobalState* d);
  static Light* createInstance(std::string_view subtype, ViskoresDeviceGlobalState* d);
};

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_SPECIALIZATION(viskores_device::Light*, ANARI_LIGHT);
