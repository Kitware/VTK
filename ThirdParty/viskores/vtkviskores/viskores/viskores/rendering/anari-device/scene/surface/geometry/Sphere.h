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
#include "array/Array1D.h"

namespace viskores_device
{

struct Sphere : public Geometry
{
  Sphere(ViskoresDeviceGlobalState* s);

  void commitParameters() override;
  void finalize() override;

  virtual void render(
    viskores::rendering::Canvas& canvas,
    const viskores::rendering::Camera& camera,
    const viskores::cont::Field& field,
    const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const override;

private:
  void SetupIndexBased();

  helium::ChangeObserverPtr<Array1D> m_index;
  FieldArrayParameters m_vertexAttributes;

  viskores::cont::ColorTable m_colorTable;

  // TODO: Add these later.
  // std::array<helium::IntrusivePtr<Array1D>, 5> m_vertexAttributes;
  // std::vector<uint32_t> m_attributeIndex;

  float m_globalRadius{ 0.f }; // fallback radius if m_vertexRadius not there.
  viskores::Range m_radiusRange;
};

} // namespace viskores_device
