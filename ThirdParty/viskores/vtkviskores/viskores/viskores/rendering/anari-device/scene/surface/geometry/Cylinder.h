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

#include <viskores/cont/ArrayHandle.h>

#include <memory>
#include <string>

namespace viskores
{
namespace rendering
{
namespace raytracing
{
class CylinderIntersector;
}
}
}

namespace viskores_device
{

struct Cylinder : public Geometry
{
  Cylinder(ViskoresDeviceGlobalState* s);

  void commitParameters() override;
  void finalize() override;

  bool isValid() const override;

  virtual void render(
    viskores::rendering::Canvas& canvas,
    const viskores::rendering::Camera& camera,
    const viskores::cont::Field& field,
    const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const override;

private:
  viskores::UInt8 ParseCaps(const std::string& caps);

  void BuildRadii(viskores::Id numberOfCylinders);

  void BuildCapMasks();

  helium::ChangeObserverPtr<Array1D> m_index;
  helium::ChangeObserverPtr<Array1D> m_radius;
  helium::ChangeObserverPtr<Array1D> m_caps;
  FieldArrayParameters m_vertexAttributes;

  viskores::cont::ArrayHandle<viskores::Id3> m_cylinderIds;
  viskores::cont::ArrayHandle<viskores::Float32> m_radii;
  viskores::cont::ArrayHandle<viskores::UInt8> m_capMasks;
  std::shared_ptr<viskores::rendering::raytracing::CylinderIntersector> m_cylinderIntersector;

  float m_globalRadius{ 0.01f };
  viskores::UInt8 m_globalCapMask{ 0 };
};

} // namespace viskores_device
