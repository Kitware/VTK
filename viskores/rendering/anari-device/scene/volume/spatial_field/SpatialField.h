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
// Viskores
#include <viskores/cont/DataSet.h>

namespace viskores_device
{

struct SpatialField : public Object
{
  SpatialField(ViskoresDeviceGlobalState* d);
  virtual ~SpatialField();
  static SpatialField* createInstance(std::string_view subtype, ViskoresDeviceGlobalState* d);

  viskores::cont::DataSet getDataSet() const { return this->m_dataSet; }

protected:
  viskores::cont::DataSet m_dataSet;
};

struct UnknownSpatialField : public SpatialField
{
  UnknownSpatialField(ViskoresDeviceGlobalState* d);

  void commitParameters() override;
  void finalize() override;

  bool isValid() const override;
};

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_SPECIALIZATION(viskores_device::SpatialField*, ANARI_SPATIAL_FIELD);
