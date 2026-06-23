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

#include "Material.h"
#include "sampler/Sampler.h"
// helium
#include <helium/utility/ChangeObserverPtr.h>

namespace viskores_device
{

struct MatteMaterial : public Material
{
  MatteMaterial(ViskoresDeviceGlobalState* d);

  void commitParameters() override;
  void finalize() override;

  Sampler* sampler() const { return this->m_sampler.get(); }

  helium::Attribute colorAttribute() const { return this->m_colorAttribute; }
  const viskores::Vec3f_32& color() const { return this->m_color; }

  helium::Attribute opacityAttribute() const { return this->m_opacityAttribute; }
  viskores::Float32 opacity() const { return this->m_opacity; }

  helium::AlphaMode alphaMode() const { return this->m_alphaMode; }
  viskores::Float32 alphaCutoff() const { return this->m_alphaCutoff; }

  void getColors(const viskores::cont::DataSet& data,
                 viskores::cont::Field& field,
                 viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const override;

private:
  helium::ChangeObserverPtr<Sampler> m_sampler;
  helium::Attribute m_colorAttribute;
  viskores::Vec3f_32 m_color;

  helium::Attribute m_opacityAttribute;
  viskores::Float32 m_opacity;

  helium::AlphaMode m_alphaMode;
  viskores::Float32 m_alphaCutoff;
};

} // namespace viskores_device
