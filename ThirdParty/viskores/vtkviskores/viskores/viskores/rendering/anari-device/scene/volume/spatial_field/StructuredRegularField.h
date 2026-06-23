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

#include "SpatialField.h"
#include "array/Array3D.h"
// helium
#include <helium/utility/ChangeObserverPtr.h>

namespace viskores_device
{

struct StructuredRegularField : public SpatialField
{
  StructuredRegularField(ViskoresDeviceGlobalState* d);

  void commitParameters() override;
  void finalize() override;

private:
  helium::ChangeObserverPtr<Array3D> m_dataArray;
};

} // namespace viskores_device
