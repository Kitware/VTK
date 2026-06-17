//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Surface.h"

namespace viskores_device
{

Surface::Surface(ViskoresDeviceGlobalState* s)
  : Object(ANARI_SURFACE, s)
  , m_geometry(this)
  , m_material(this)
{
}

Surface::~Surface() = default;

void Surface::commitParameters()
{
  m_id = getParam<uint32_t>("id", ~0u);
  m_geometry = getParamObject<Geometry>("geometry");
  m_material = getParamObject<Material>("material");
}

void Surface::finalize()
{
  if (!this->m_material || !this->m_material->isValid())
  {
    reportMessage(ANARI_SEVERITY_WARNING, "missing 'material' on ANARISurface");
    return;
  }

  if (!this->m_geometry || !this->m_geometry->isValid())
  {
    reportMessage(ANARI_SEVERITY_WARNING, "missing 'geometry' on ANARISurface");
    return;
  }

  this->m_dataSet = this->m_geometry->getDataSet();
  this->m_material->getColors(this->m_dataSet, this->m_field, this->m_colorMap);
}

const Geometry* Surface::geometry() const
{
  return m_geometry.get();
}

const Material* Surface::material() const
{
  return m_material.get();
}

void Surface::render(viskores::rendering::Canvas& canvas,
                     const viskores::rendering::Camera& camera) const
{
  this->m_geometry->render(canvas, camera, this->m_field, this->m_colorMap);
}

viskores::Bounds Surface::bounds() const
{
  return this->geometry()->getDataSet().GetCoordinateSystem().GetBounds();
}

bool Surface::isValid() const
{
  return m_geometry && m_material && m_geometry->isValid() && m_material->isValid();
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Surface*);
