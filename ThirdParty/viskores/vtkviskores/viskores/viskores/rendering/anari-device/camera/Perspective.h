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

#include "Camera.h"

namespace viskores_device
{

struct Perspective : public Camera
{
  Perspective(ViskoresDeviceGlobalState* s);
  void commitParameters() override;
  void finalize() override;

  viskores::rendering::Camera camera(const viskores::Bounds& bounds) const override;

private:
  viskores::Float32 m_fovy;
  viskores::Float32 m_aspect;
  viskores::Float32 m_near;
  viskores::Float32 m_far;
};

} // namespace viskores_device
