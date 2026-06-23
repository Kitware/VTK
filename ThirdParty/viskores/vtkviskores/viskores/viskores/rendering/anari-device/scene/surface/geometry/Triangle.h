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

#include "Geometry.h"

namespace viskores_device
{

struct Triangle : Geometry
{
  Triangle(ViskoresDeviceGlobalState* s);

  void commitParameters() override;
  void finalize() override;

  bool isValid() const override;

  virtual void render(
    viskores::rendering::Canvas& canvas,
    const viskores::rendering::Camera& camera,
    const viskores::cont::Field& field,
    const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const override;

private:
  helium::ChangeObserverPtr<Array1D> m_index;
  FieldArrayParameters m_vertexAttributes;
  FieldArrayParameters m_faceVaryingAttributes;

  viskores::cont::ColorTable m_colorTable;
  helium::ChangeObserverPtr<Array1D> m_vertexColor;
};

} // namespace viskores_device
