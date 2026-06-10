//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Light.h"

namespace viskores_device
{

Light::Light(ViskoresDeviceGlobalState* s)
  : Object(ANARI_LIGHT, s)
{
}

Light* Light::createInstance(std::string_view /*subtype*/, ViskoresDeviceGlobalState* s)
{
  return (Light*)new UnknownObject(ANARI_LIGHT, s);
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Light*);
