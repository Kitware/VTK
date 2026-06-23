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

#include "../ViskoresTypes.h"
#include "Object.h"
#include "scene/World.h"

namespace viskores_device
{

struct PixelSample
{
  float4 color;
  float depth;
  uint32_t primId{ ~0u };
  uint32_t objId{ ~0u };
  uint32_t instId{ ~0u };
};

struct Renderer : public Object
{
  Renderer(ViskoresDeviceGlobalState* s);
  ~Renderer() override;
  static Renderer* createInstance(std::string_view subtype, ViskoresDeviceGlobalState* d);
  void commitParameters() override;
  void finalize() override;

  float4 background() const;

private:
  //float4 m_bgColor{float3(0.f), 1.f};
  float4 m_bgColor{ 0.f, 0.f, 0.f, 1.f };
};

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_SPECIALIZATION(viskores_device::Renderer*, ANARI_RENDERER);
