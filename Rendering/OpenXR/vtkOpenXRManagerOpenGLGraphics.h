/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenXRManagerOpenGLGraphics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenXRManagerOpenGLGraphics

 * @brief   OpenXR manager OpenGL graphics implementation
 *
 * Allow vtkOpenXRManager to use an OpenGL rendering backend.
 */

#ifndef vtkOpenXRManagerOpenGLGraphics_h
#define vtkOpenXRManagerOpenGLGraphics_h

#include "vtkOpenXRManagerGraphics.h"
#include "vtkRenderingOpenXRModule.h" // For export macro

#include <memory> // For shared_ptr
#include <vector> // For std::vector

struct XrGraphicsBindingOpenGLWin32KHR;
struct XrSwapchainImageOpenGLKHR;

class VTKRENDERINGOPENXR_EXPORT vtkOpenXRManagerOpenGLGraphics : public vtkOpenXRManagerGraphics
{
public:
  static vtkOpenXRManagerOpenGLGraphics* New();
  vtkTypeMacro(vtkOpenXRManagerOpenGLGraphics, vtkOpenXRManagerGraphics);

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
  vtkOpenXRManagerOpenGLGraphics();
  ~vtkOpenXRManagerOpenGLGraphics() override;

  /**
   * OpenGL structure to store swapchain images.
   */
  struct SwapchainImagesOpenGL
  {
    std::vector<XrSwapchainImageOpenGLKHR> Images;
  };

  /**
   * Acquire OpenGL swapchain images an store them in \p swapchainImages.
   */
  void EnumerateSwapchainImages(XrSwapchain swapchain, SwapchainImagesOpenGL& swapchainImages);

  std::shared_ptr<XrGraphicsBindingOpenGLWin32KHR> GraphicsBinding;

private:
  vtkOpenXRManagerOpenGLGraphics(const vtkOpenXRManagerOpenGLGraphics&) = delete;
  void operator=(const vtkOpenXRManagerOpenGLGraphics&) = delete;

  class PIMPL;
  PIMPL* Private;
};

#endif
