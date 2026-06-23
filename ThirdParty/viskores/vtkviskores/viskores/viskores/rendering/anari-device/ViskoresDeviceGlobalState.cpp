//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "ViskoresDeviceGlobalState.h"
#include "frame/Frame.h"

namespace viskores_device
{

ViskoresDeviceGlobalState::ViskoresDeviceGlobalState(ANARIDevice d)
  : helium::BaseGlobalDeviceState(d)
{
}

void ViskoresDeviceGlobalState::waitOnCurrentFrame() const
{
  if (currentFrame)
    currentFrame->wait();
}

} // namespace viskores_device
