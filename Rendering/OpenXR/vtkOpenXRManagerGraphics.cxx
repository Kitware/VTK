// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenXRManagerGraphics.h"

#include "vtkOpenXRManager.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
uint32_t vtkOpenXRManagerGraphics::GetChainLength(XrSwapchain swapchain)
{
  uint32_t chainLength;
  vtkOpenXRManager::GetInstance().XrCheckOutput(vtkOpenXRManager::ErrorOutput,
    xrEnumerateSwapchainImages(swapchain, 0, &chainLength, nullptr),
    "Failed to get swapchain images count");

  return chainLength;
}
VTK_ABI_NAMESPACE_END
