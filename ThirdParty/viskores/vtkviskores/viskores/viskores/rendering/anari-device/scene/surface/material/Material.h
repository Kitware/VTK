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

#include "Object.h"
#include "sampler/Sampler.h"

#include <viskores/cont/ColorTable.h>
#include <viskores/cont/DataSet.h>

namespace viskores_device
{

struct Material : public Object
{
  Material(ViskoresDeviceGlobalState* s);
  ~Material() override;
  static Material* createInstance(std::string_view subtype, ViskoresDeviceGlobalState* s);

  virtual void getColors(const viskores::cont::DataSet& data,
                         viskores::cont::Field& field,
                         viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const = 0;
};

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_SPECIALIZATION(viskores_device::Material*, ANARI_MATERIAL);
