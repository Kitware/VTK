//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "MatteMaterial.h"

#include <viskores/cont/ArrayHandleConstant.h>

namespace viskores_device
{

MatteMaterial::MatteMaterial(ViskoresDeviceGlobalState* d)
  : Material(d)
  , m_sampler(this)
{
}

void MatteMaterial::commitParameters()
{
  this->m_sampler = this->getParamObject<Sampler>("color");
  this->m_colorAttribute = helium::attributeFromString(this->getParamString("color", "none"));
  this->m_color = this->getParam("color", viskores::Vec3f_32{ 0.8, 0.8, 0.8 });
  this->m_color = viskores::Min(viskores::Max(this->m_color, { 0.f, 0.f, 0.f }), { 1.f, 1.f, 1.f });
  // TODO: Implement sampler

  this->m_opacityAttribute = helium::attributeFromString(this->getParamString("color", "none"));
  this->m_opacity = this->getParam("opacity", viskores::Float32{ 1.0f });
  // TODO: Implement sampler

  this->m_alphaMode = helium::alphaModeFromString(this->getParamString("alphaMode", "opaque"));

  this->m_alphaCutoff = this->getParam("alphaCutoff", viskores::Float32{ 0.5f });
}

void MatteMaterial::finalize()
{
  // no-op
}

void MatteMaterial::getColors(const viskores::cont::DataSet& data,
                              viskores::cont::Field& field,
                              viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const
{
  if (this->m_sampler && this->m_sampler->isValid())
  {
    if (this->m_sampler->getColors(data, field, colorMap))
    {
      return;
    }
  }

  // TODO: Implement sampling and attributes.
  // This should be the fallback when other coloring is missing.
  colorMap.Allocate(1);
  colorMap.WritePortal().Set(
    0, { this->m_color[0], this->m_color[1], this->m_color[2], this->m_opacity });
  field = viskores::cont::Field{ "data",
                                 viskores::cont::Field::Association::Points,
                                 viskores::cont::make_ArrayHandleConstant(
                                   viskores::Float32{ 0.0f }, data.GetNumberOfPoints()) };
}

} // namespace viskores_device
