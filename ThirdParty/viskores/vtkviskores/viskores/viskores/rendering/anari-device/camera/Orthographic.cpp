//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Orthographic.h"

namespace viskores_device
{

Orthographic::Orthographic(ViskoresDeviceGlobalState* s)
  : Camera(s)
{
}

void Orthographic::commitParameters()
{
  Camera::commitParameters();

  this->m_aspect = this->getParam("aspect", viskores::Float32(1));
  this->m_height = this->getParam("height", viskores::Float32(1));
  this->m_near = this->getParam("near", viskores::Float32(-1));
  this->m_far = this->getParam("far", viskores::Float32(-1));
}

void Orthographic::finalize()
{
  Camera::finalize();
}

viskores::rendering::Camera Orthographic::camera(const viskores::Bounds& bounds) const
{
  // TODO: Implement orthographic projection correctly
  viskores::rendering::Camera camera;

  viskores::Vec3f_64 diagonal = { bounds.X.Length(), bounds.Y.Length(), bounds.Z.Length() };
  viskores::Float32 length = static_cast<viskores::Float32>(viskores::Magnitude(diagonal));

  camera.SetPosition(this->m_position);
  camera.SetLookAt(this->m_position + (length * this->m_direction));
  camera.SetViewUp(this->m_up);
  camera.SetClippingRange((this->m_near > 0) ? this->m_near : (0.001f * length),
                          (this->m_far > 0) ? this->m_far : (100.f * length));
#if 0
  camera.SetViewport(this->m_imageRegion[0],
                     this->m_imageRegion[2],
                     this->m_imageRegion[1],
                     this->m_imageRegion[3]);
#endif

  return camera;
}

} // namespace viskores_device
