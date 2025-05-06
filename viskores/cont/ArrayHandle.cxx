//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/ArrayHandle.h>

namespace viskores
{
namespace cont
{
namespace detail
{

VISKORES_CONT void ArrayHandleReleaseResourcesExecution(
  const std::vector<viskores::cont::internal::Buffer>& buffers)
{
  viskores::cont::Token token;

  for (auto&& buf : buffers)
  {
    buf.ReleaseDeviceResources();
  }
}

VISKORES_CONT bool ArrayHandleIsOnDevice(
  const std::vector<viskores::cont::internal::Buffer>& buffers,
  viskores::cont::DeviceAdapterId device)
{
  for (auto&& buf : buffers)
  {
    if (!buf.IsAllocatedOnDevice(device))
    {
      return false;
    }
  }
  return true;
}
}
}
} // namespace viskores::cont::detail
