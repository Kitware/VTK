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

struct XrGraphicsBindingD3D11KHR;
struct XrSwapchainImageD3D11KHR;

class VTKRENDERINGOPENXRREMOTING_EXPORT vtkOpenXRManagerD3DGraphics
  : public vtkOpenXRManagerGraphics
{
public:
  static vtkOpenXRManagerD3DGraphics* New();
  vtkTypeMacro(vtkOpenXRManagerD3DGraphics, vtkOpenXRManagerGraphics);

  void SetNumberOfSwapchains(uint32_t viewCount) override;

  void GetColorSwapchainImage(uint32_t scIndex, uint32_t imgIndex, void* texture) override;

  void GetDepthSwapchainImage(uint32_t scIndex, uint32_t imgIndex, void* texture) override;

  void EnumerateColorSwapchainImages(XrSwapchain swapchain, uint32_t scIndex) override;

  void EnumerateDepthSwapchainImages(XrSwapchain swapchain, uint32_t scIndex) override;

  const std::vector<int64_t>& GetSupportedColorFormats() override;

  const std::vector<int64_t>& GetSupportedDepthFormats() override;

  bool CreateGraphicsBinding(vtkOpenGLRenderWindow* helperWindow) override;

  const void* GetGraphicsBinding() override { return this->GraphicsBinding.get(); };

  bool CheckGraphicsRequirements(XrInstance instance, XrSystemId id) override;

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

#endif
