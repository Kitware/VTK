//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Renderer.h"
#include "../ViskoresTypes.h"

namespace viskores_device
{

Renderer::Renderer(ViskoresDeviceGlobalState* s)
  : Object(ANARI_RENDERER, s)
{
}

Renderer::~Renderer() = default;

Renderer* Renderer::createInstance(std::string_view /* subtype */, ViskoresDeviceGlobalState* s)
{
  return new Renderer(s);
}

void Renderer::commitParameters()
{
  m_bgColor = getParam<float4>("background", float4(0.f, 0.f, 0.f, 1.f));
}

void Renderer::finalize()
{
  // no-op
}

float4 Renderer::background() const
{
  return m_bgColor;
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Renderer*);
