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
#include "ViskoresTypes.h"
// Viskores
#include <viskores/Bounds.h>
#include <viskores/cont/DataSet.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Canvas.h>

namespace viskores_device
{

struct Volume : public Object
{
  Volume(ViskoresDeviceGlobalState* d);
  virtual ~Volume();
  static Volume* createInstance(std::string_view subtype, ViskoresDeviceGlobalState* d);

  void commitParameters() override;
  void finalize() override;

  virtual void render(viskores::rendering::Canvas& canvas,
                      const viskores::rendering::Camera& camera) const = 0;

  uint32_t id() const;

  virtual viskores::Bounds bounds() const = 0;

private:
  uint32_t m_id{ ~0u };
};

// Inlined definitions ////////////////////////////////////////////////////////

inline uint32_t Volume::id() const
{
  return m_id;
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_SPECIALIZATION(viskores_device::Volume*, ANARI_VOLUME);
