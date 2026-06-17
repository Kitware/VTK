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

#include "Volume.h"
#include "array/Array1D.h"
#include "spatial_field/SpatialField.h"
// helium
#include <helium/utility/ChangeObserverPtr.h>
// Viskores
#include <viskores/Range.h>
#include <viskores/cont/ColorTable.h>

namespace viskores_device
{

struct TransferFunction1D : public Volume
{
  TransferFunction1D(ViskoresDeviceGlobalState* d);

  void commitParameters() override;
  void finalize() override;

  void render(viskores::rendering::Canvas& canvas,
              const viskores::rendering::Camera& camera) const override;

  const SpatialField* spatialField() const;
  viskores::Bounds bounds() const override;

  bool isValid() const override;

private:
  helium::ChangeObserverPtr<SpatialField> m_spatialField;
  viskores::Range m_valueRange;
  helium::ChangeObserverPtr<Array1D> m_colorArray;
  helium::ChangeObserverPtr<Array1D> m_opacityArray;
  float4 m_color;
  viskores::Float32 m_alpha;
  viskores::Float32 m_unitDistance;
  viskores::Float32 m_sampleDistance;
  viskores::cont::ArrayHandle<viskores::Vec4f_32> m_colorMap;
};

} // namespace viskores_device
