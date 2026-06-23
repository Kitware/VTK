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

#include "../ViskoresDeviceGlobalState.h"
#include "Array1D.h"
// helium
#include "helium/array/ObjectArray.h"

namespace viskores_device
{

struct ObjectArray : public helium::ObjectArray
{
  ObjectArray(ViskoresDeviceGlobalState* state, const Array1DMemoryDescriptor& d);
  ~ObjectArray() override;
};

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_SPECIALIZATION(viskores_device::ObjectArray*, ANARI_ARRAY1D);
