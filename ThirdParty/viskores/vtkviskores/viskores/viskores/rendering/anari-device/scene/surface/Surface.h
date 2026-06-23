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

#include "geometry/Geometry.h"
#include "material/Material.h"
// Viskores
#include <viskores/Bounds.h>
#include <viskores/cont/ColorTable.h>
#include <viskores/cont/Field.h>

namespace viskores_device
{

struct Surface : public Object
{
  Surface(ViskoresDeviceGlobalState* s);
  ~Surface() override;

  void commitParameters() override;
  void finalize() override;

  uint32_t id() const { return m_id; }
  const Geometry* geometry() const;
  const Material* material() const;

  void render(viskores::rendering::Canvas& canvas, const viskores::rendering::Camera& camera) const;

  viskores::Bounds bounds() const;

  bool isValid() const override;

private:
  uint32_t m_id{ ~0u };
  helium::ChangeObserverPtr<Geometry> m_geometry;
  helium::ChangeObserverPtr<Material> m_material;

  viskores::cont::DataSet m_dataSet;
  viskores::cont::Field m_field;
  viskores::cont::ArrayHandle<viskores::Vec4f_32> m_colorMap;
};

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_SPECIALIZATION(viskores_device::Surface*, ANARI_SURFACE);
