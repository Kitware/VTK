//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Camera.h"
// specific types
#include "Orthographic.h"
#include "Perspective.h"

namespace viskores_device
{

Camera::Camera(ViskoresDeviceGlobalState* s)
  : Object(ANARI_CAMERA, s)
{
}

Camera::~Camera() = default;

Camera* Camera::createInstance(std::string_view type, ViskoresDeviceGlobalState* s)
{
  if (type == "perspective")
    return new Perspective(s);
  else if (type == "orthographic")
    return new Orthographic(s);
  else
    return new UnknownCamera(s);
}

void Camera::commitParameters()
{
  this->m_position = getParam<viskores::Vec3f_32>("position", { 0.f, 0.f, 0.f });
  this->m_direction =
    viskores::Normal(getParam<viskores::Vec3f_32>("direction", { 0.f, 0.f, -1.f }));
  this->m_up = viskores::Normal(getParam<viskores::Vec3f_32>("up", { 0.f, 1.f, 0.f }));
  this->m_imageRegion = viskores::Vec4f_32(0.f, 0.f, 1.f, 1.f);
  getParam("imageRegion", ANARI_FLOAT32_BOX2, &this->m_imageRegion);

#if 0
  std::cout << "Pos== " << this->m_position << std::endl;
  std::cout << "Dir== " << this->m_direction << std::endl;
  std::cout << "Up== " << this->m_up << std::endl;
#endif

  this->markUpdated();
}

void Camera::finalize()
{
  // no-op
}

UnknownCamera::UnknownCamera(ViskoresDeviceGlobalState* s)
  : Camera(s){};

viskores::rendering::Camera UnknownCamera::camera(const viskores::Bounds&) const
{
  return {};
}

bool UnknownCamera::isValid() const
{
  return false;
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Camera*);
