/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenXRManagerGraphics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenXRManagerGraphics.h"

#include "vtkOpenXRManager.h"

//------------------------------------------------------------------------------
uint32_t vtkOpenXRManagerGraphics::GetChainLength(XrSwapchain swapchain)
{
  uint32_t chainLength;
  vtkOpenXRManager::GetInstance().XrCheckError(
    xrEnumerateSwapchainImages(swapchain, 0, &chainLength, nullptr),
    "Failed to get swapchain images count");

  return chainLength;
}
