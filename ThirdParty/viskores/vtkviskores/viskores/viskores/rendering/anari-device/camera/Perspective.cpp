//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Perspective.h"

namespace viskores_device
{

Perspective::Perspective(ViskoresDeviceGlobalState* s)
  : Camera(s)
{
}

void Perspective::commitParameters()
{
  Camera::commitParameters();

  this->m_fovy = this->getParam("fovy", viskores::Pi_3f());
  this->m_aspect = this->getParam("aspect", viskores::Float32(1));
  this->m_near = this->getParam("near", viskores::Float32(-1));
  this->m_far = this->getParam("far", viskores::Float32(-1));
}

void Perspective::finalize()
{
  Camera::finalize();
}

viskores::rendering::Camera Perspective::camera(const viskores::Bounds& bounds) const
{
  viskores::rendering::Camera camera;

  viskores::Vec3f_64 diagonal = { bounds.X.Length(), bounds.Y.Length(), bounds.Z.Length() };
  viskores::Float32 length = static_cast<viskores::Float32>(viskores::Magnitude(diagonal));

  camera.SetPosition(this->m_position);
  camera.SetLookAt(this->m_position + (length * this->m_direction));
  camera.SetViewUp(this->m_up);
  camera.SetFieldOfView(anari::degrees(this->m_fovy));
  camera.SetClippingRange((this->m_near > 0) ? this->m_near : (0.01f * length),
                          (this->m_far > 0) ? this->m_far : (1000.f * length));
#if 0
  camera.SetViewport(this->m_imageRegion[0],
                     this->m_imageRegion[2],
                     this->m_imageRegion[1],
                     this->m_imageRegion[3]);
#endif

  // TODO: The aspect parameter is ignored. This is handled elsewhere

  return camera;
}

} // namespace viskores_device
