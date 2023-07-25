// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

struct XrGraphicsBindingD3D11KHR;
struct XrSwapchainImageD3D11KHR;

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENXRREMOTING_EXPORT vtkOpenXRManagerD3DGraphics
  : public vtkOpenXRManagerGraphics
{
public:
  static vtkOpenXRManagerD3DGraphics* New();
  vtkTypeMacro(vtkOpenXRManagerD3DGraphics, vtkOpenXRManagerGraphics);

  /**
   * Resize the internal vectors storing the color and depth swapchains.
   */
  void SetNumberOfSwapchains(uint32_t viewCount) override;

  ///@{
  /**
   * Fill \p texture with the D3D Texture2D for the specified eye \p scIndex.
   * The image index \p imgIndex should be obtained beforehand using xrAcquireSwapchainImage.
   */
  void GetColorSwapchainImage(uint32_t scIndex, uint32_t imgIndex, void* texture) override;
  void GetDepthSwapchainImage(uint32_t scIndex, uint32_t imgIndex, void* texture) override;
  ///@}

  ///@{
  /**
   * Acquire D3D swapchain images for the specified eye index.
   */
  void EnumerateColorSwapchainImages(XrSwapchain swapchain, uint32_t scIndex) override;
  void EnumerateDepthSwapchainImages(XrSwapchain swapchain, uint32_t scIndex) override;
  ///@}

  ///@{
  /**
   * Return the list of DXGI_FORMAT supported by vtkWin32OpenGLDXRenderWindow.
   * The first in the list that is also supported by the runtime is picked.
   */
  const std::vector<int64_t>& GetSupportedColorFormats() override;
  const std::vector<int64_t>& GetSupportedDepthFormats() override;
  ///@}

  /**
   * Fill the pointer to the XrGraphicsBindingD3D11 structure.
   */
  bool CreateGraphicsBinding(vtkOpenGLRenderWindow* helperWindow) override;

  /**
   * Return pointer to the XrGraphicsBindingD3D11 structure required to create the OpenXR session.
   */
  const void* GetGraphicsBinding() override { return this->GraphicsBinding.get(); };

  /**
   * Verify that the D3D feature levels supported by the runtime match the ones supported by
   * vtkWin32OpenGLDXRenderWindow.
   */
  bool CheckGraphicsRequirements(XrInstance instance, XrSystemId id) override;

  /**
   * Return the extension name corresponding to the D3D11 rendering backend
   */
  const char* GetBackendExtensionName() override;

protected:
  vtkOpenXRManagerD3DGraphics();
  ~vtkOpenXRManagerD3DGraphics() override;

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

  std::shared_ptr<XrGraphicsBindingD3D11KHR> GraphicsBinding;

private:
  vtkOpenXRManagerD3DGraphics(const vtkOpenXRManagerD3DGraphics&) = delete;
  void operator=(const vtkOpenXRManagerD3DGraphics&) = delete;

  class PIMPL;
  PIMPL* Private;
};

VTK_ABI_NAMESPACE_END
#endif
