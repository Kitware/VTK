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

#define XR_USE_GRAPHICS_API_D3D11
#include "d3d11.h" // Required headers for the XrGraphicsRequirementsD3D11KHR struct
#include <vtkOpenXRPlatform.h>

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenXRManagerD3DGraphics::PIMPL
{
public:
  // D3D swapchains
  std::vector<SwapchainImagesD3D> ColorSwapchains;
  std::vector<SwapchainImagesD3D> DepthSwapchains;
};

vtkStandardNewMacro(vtkOpenXRManagerD3DGraphics);

//------------------------------------------------------------------------------
vtkOpenXRManagerD3DGraphics::vtkOpenXRManagerD3DGraphics()
{
  this->Private = new PIMPL;
}

//------------------------------------------------------------------------------
vtkOpenXRManagerD3DGraphics::~vtkOpenXRManagerD3DGraphics()
{
  delete this->Private;
}

//------------------------------------------------------------------------------
void vtkOpenXRManagerD3DGraphics::SetNumberOfSwapchains(uint32_t viewCount)
{
  this->Private->ColorSwapchains.resize(viewCount);
  this->Private->DepthSwapchains.resize(viewCount);
}

//------------------------------------------------------------------------------
void vtkOpenXRManagerD3DGraphics::GetColorSwapchainImage(
  uint32_t scIndex, uint32_t imgIndex, void* texture)
{
  *(ID3D11Texture2D**)texture = this->Private->ColorSwapchains[scIndex].Images[imgIndex].texture;
}

//------------------------------------------------------------------------------
void vtkOpenXRManagerD3DGraphics::GetDepthSwapchainImage(
  uint32_t scIndex, uint32_t imgIndex, void* texture)
{
  *(ID3D11Texture2D**)texture = this->Private->DepthSwapchains[scIndex].Images[imgIndex].texture;
}

//------------------------------------------------------------------------------
void vtkOpenXRManagerD3DGraphics::EnumerateColorSwapchainImages(
  XrSwapchain swapchain, uint32_t scIndex)
{
  this->EnumerateSwapchainImages(swapchain, this->Private->ColorSwapchains[scIndex]);
}

//------------------------------------------------------------------------------
void vtkOpenXRManagerD3DGraphics::EnumerateDepthSwapchainImages(
  XrSwapchain swapchain, uint32_t scIndex)
{
  this->EnumerateSwapchainImages(swapchain, this->Private->DepthSwapchains[scIndex]);
}

//------------------------------------------------------------------------------
const std::vector<int64_t>& vtkOpenXRManagerD3DGraphics::GetSupportedColorFormats()
{
  const static std::vector<int64_t> supportedColorFormats = { DXGI_FORMAT_R8G8B8A8_UNORM };
  return supportedColorFormats;
}

//------------------------------------------------------------------------------
const std::vector<int64_t>& vtkOpenXRManagerD3DGraphics::GetSupportedDepthFormats()
{
  const static std::vector<int64_t> supportedDepthFormats = { DXGI_FORMAT_D16_UNORM,
    DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_D32_FLOAT_S8X24_UINT };
  return supportedDepthFormats;
}

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

    graphicsBindingDXWin32->device = d3dWindow->GetDevice();
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManagerD3DGraphics::CheckGraphicsRequirements(XrInstance instance, XrSystemId id)
{
  XrGraphicsRequirementsD3D11KHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };

  xr::GraphicsExtensionDispatchTable extensions;
  extensions.PopulateDispatchTable(instance);

  if (!vtkOpenXRManager::GetInstance().XrCheckError(
        extensions.xrGetD3D11GraphicsRequirementsKHR(instance, id, &graphicsRequirements),
        "Failed to get DirectX graphics requirements!"))
  {
    return false;
  }

  // Create a list of feature levels which are both supported by the OpenXR runtime and this
  // application. vtkWin32OpenGLDXRenderWindow only supports D3D11 for now.
  constexpr D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1 };
  auto supportedLevel =
    std::lower_bound(std::begin(levels), std::end(levels), graphicsRequirements.minFeatureLevel);
  if (supportedLevel == std::end(levels))
  {
    vtkErrorMacro("Unsupported minimum feature level!");
    return false;
  };

  return true;
}

//------------------------------------------------------------------------------
const char* vtkOpenXRManagerD3DGraphics::GetBackendExtensionName()
{
  return XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
}
VTK_ABI_NAMESPACE_END
