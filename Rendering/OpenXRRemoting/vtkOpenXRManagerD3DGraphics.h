/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenXRManagerD3DGraphics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenXRManagerD3DGraphics

 * @brief   OpenXR manager D3D graphics implementation
 *
 * Allow vtkOpenXRManager to use a D3D rendering backend.
 */

#ifndef vtkOpenXRManagerD3DGraphics_h
#define vtkOpenXRManagerD3DGraphics_h

#include "vtkOpenXRManagerGraphics.h"
#include "vtkRenderingOpenXRRemotingModule.h" // For export macro

#include "vtkOpenXR.h" // For extension name

#include <memory> // For shared_ptr
#include <vector> // For std::vector

class VTKRENDERINGOPENXRREMOTING_EXPORT vtkOpenXRManagerD3DGraphics
  : public vtkOpenXRManagerGraphics
{
public:
  static vtkOpenXRManagerD3DGraphics* New();
  vtkTypeMacro(vtkOpenXRManagerD3DGraphics, vtkOpenXRManagerGraphics);

  virtual void SetNumberOfSwapchains(uint32_t viewCount)
  {
    this->ColorSwapchains.resize(viewCount);
    this->DepthSwapchains.resize(viewCount);
  };

  void GetColorSwapchainImage(uint32_t scIndex, uint32_t imgIndex, void* texture)
  {
    *(ID3D11Texture2D**)texture = this->ColorSwapchains[scIndex].Images[imgIndex].texture;
  };

  void GetDepthSwapchainImage(uint32_t scIndex, uint32_t imgIndex, void* texture)
  {
    *(ID3D11Texture2D**)texture = this->DepthSwapchains[scIndex].Images[imgIndex].texture;
  };

  void EnumerateColorSwapchainImages(XrSwapchain swapchain, uint32_t scIndex)
  {
    this->EnumerateSwapchainImages(swapchain, this->ColorSwapchains[scIndex]);
  };

  void EnumerateDepthSwapchainImages(XrSwapchain swapchain, uint32_t scIndex)
  {
    this->EnumerateSwapchainImages(swapchain, this->DepthSwapchains[scIndex]);
  };

  const std::vector<int64_t>& GetSupportedColorFormats()
  {
    const static std::vector<int64_t> supportedColorFormats = { DXGI_FORMAT_R8G8B8A8_UNORM };
    return supportedColorFormats;
  }

  const std::vector<int64_t>& GetSupportedDepthFormats()
  {
    const static std::vector<int64_t> supportedDepthFormats = { DXGI_FORMAT_D16_UNORM,
      DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_D32_FLOAT_S8X24_UINT };
    return supportedDepthFormats;
  }

  bool CreateGraphicsBinding(vtkOpenGLRenderWindow* helperWindow);

  const void* GetGraphicsBinding() { return this->GraphicsBinding.get(); };

  bool CheckGraphicsRequirements(
    XrInstance instance, XrSystemId id, xr::ExtensionDispatchTable extensions);

  const char* GetBackendExtensionName() { return XR_KHR_D3D11_ENABLE_EXTENSION_NAME; };

protected:
  vtkOpenXRManagerD3DGraphics() = default;
  ~vtkOpenXRManagerD3DGraphics() = default;

  /**
   * D3D structure to store swapchain images.
   */
  struct SwapchainImagesD3D
  {
    std::vector<XrSwapchainImageD3D11KHR> Images;
  };

  /**
   * Acquire D3D swapchain images an store them in \p swapchainImages.
   */
  void EnumerateSwapchainImages(XrSwapchain swapchain, SwapchainImagesD3D& swapchainImages);

  // D3D swapchains
  std::vector<SwapchainImagesD3D> ColorSwapchains;
  std::vector<SwapchainImagesD3D> DepthSwapchains;

  std::shared_ptr<XrGraphicsBindingD3D11KHR> GraphicsBinding;

private:
  vtkOpenXRManagerD3DGraphics(const vtkOpenXRManagerD3DGraphics&) = delete;
  void operator=(const vtkOpenXRManagerD3DGraphics&) = delete;
};

#endif
