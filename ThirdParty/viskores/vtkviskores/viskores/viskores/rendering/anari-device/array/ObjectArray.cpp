//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "array/ObjectArray.h"
#include "../ViskoresDeviceGlobalState.h"

namespace viskores_device
{

ObjectArray::ObjectArray(ViskoresDeviceGlobalState* state, const Array1DMemoryDescriptor& d)
  : helium::ObjectArray(state, d)
{
  state->objectCounts.arrays++;
}

ObjectArray::~ObjectArray()
{
  asViskoresDeviceState(deviceState())->objectCounts.arrays--;
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::ObjectArray*);
