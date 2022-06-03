/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenXRManagerD3DGraphics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenXRManagerD3DGraphics.h"

#include "vtkObjectFactory.h"
#include "vtkOpenXRManager.h"
#include "vtkWin32OpenGLDXRenderWindow.h"

vtkStandardNewMacro(vtkOpenXRManagerD3DGraphics);

//------------------------------------------------------------------------------
void vtkOpenXRManagerD3DGraphics::EnumerateSwapchainImages(
  XrSwapchain swapchain, SwapchainImagesD3D& swapchainImages)
{
  uint32_t chainLength = this->GetChainLength(swapchain);

  swapchainImages.Images.resize(chainLength, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });

  vtkOpenXRManager::GetInstance().XrCheckError(
    xrEnumerateSwapchainImages(swapchain, (uint32_t)swapchainImages.Images.size(), &chainLength,
      reinterpret_cast<XrSwapchainImageBaseHeader*>(swapchainImages.Images.data())),
    "Failed to enumerate swapchain images");
}

//------------------------------------------------------------------------------
bool vtkOpenXRManagerD3DGraphics::CreateGraphicsBinding(vtkOpenGLRenderWindow* helperWindow)
{
  vtkWin32OpenGLDXRenderWindow* d3dWindow =
    vtkWin32OpenGLDXRenderWindow::SafeDownCast(helperWindow);
  if (d3dWindow)
  {
    auto graphicsBindingDXWin32 =
      std::shared_ptr<XrGraphicsBindingD3D11KHR>(new XrGraphicsBindingD3D11KHR{
        XR_TYPE_GRAPHICS_BINDING_D3D11_KHR, // .type
        nullptr,                            // .next
        { 0 }                               // ID3D11Device* device
      });
    this->GraphicsBinding = graphicsBindingDXWin32;

    graphicsBindingDXWin32->device = d3dWindow->GetDevice().Get();
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManagerD3DGraphics::CheckGraphicsRequirements(
  XrInstance instance, XrSystemId id, xr::ExtensionDispatchTable extensions)
{
  XrGraphicsRequirementsD3D11KHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
  if (!vtkOpenXRManager::GetInstance().XrCheckError(
        extensions.xrGetD3D11GraphicsRequirementsKHR(instance, id, &graphicsRequirements),
        "Failed to get DirectX graphics requirements!"))
  {
    return false;
  }

  // Create a list of feature levels which are both supported by the OpenXR runtime and this
  // application. vtkWin32OpenGLDXRenderWindow only supports D3D11 for now.
  std::vector<D3D_FEATURE_LEVEL> featureLevels = { D3D_FEATURE_LEVEL_11_1 };
  featureLevels.erase(
    std::remove_if(featureLevels.begin(), featureLevels.end(),
      [&](D3D_FEATURE_LEVEL fl) { return fl < graphicsRequirements.minFeatureLevel; }),
    featureLevels.end());
  if (featureLevels.empty())
  {
    vtkErrorMacro("Unsupported minimum feature level!");
  };

  return true;
}
