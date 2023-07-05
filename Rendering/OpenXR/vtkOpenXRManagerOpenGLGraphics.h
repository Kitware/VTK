// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkRenderingOpenGLConfigure.h" // For VTK_USE_X / WIN32

#include <memory> // For shared_ptr
#include <vector> // For std::vector

// Forward declare the graphics binding struct depending on the platform being used
#ifdef VTK_USE_X
struct XrGraphicsBindingOpenGLXlibKHR;
typedef XrGraphicsBindingOpenGLXlibKHR XrGraphicsBindingOpenGL;
#elif defined(_WIN32)
struct XrGraphicsBindingOpenGLWin32KHR;
typedef XrGraphicsBindingOpenGLWin32KHR XrGraphicsBindingOpenGL;
#endif
struct XrSwapchainImageOpenGLKHR;

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENXR_EXPORT vtkOpenXRManagerOpenGLGraphics : public vtkOpenXRManagerGraphics
{
public:
  static vtkOpenXRManagerOpenGLGraphics* New();
  vtkTypeMacro(vtkOpenXRManagerOpenGLGraphics, vtkOpenXRManagerGraphics);

  /**
   * Resize the internal vectors storing the color and depth swapchains.
   */
  void SetNumberOfSwapchains(uint32_t viewCount) override;

  ///@{
  /**
   * Fill \p texture with the OpenGL texture id for the specified eye \p scIndex.
   * The image index \p imgIndex should be obtained beforehand using xrAcquireSwapchainImage.
   */
  void GetColorSwapchainImage(uint32_t scIndex, uint32_t imgIndex, void* texture) override;
  void GetDepthSwapchainImage(uint32_t scIndex, uint32_t imgIndex, void* texture) override;
  ///@}

  ///@{
  /**
   * Acquire OpenGL swapchain images for the specified eye index.
   */
  void EnumerateColorSwapchainImages(XrSwapchain swapchain, uint32_t scIndex) override;
  void EnumerateDepthSwapchainImages(XrSwapchain swapchain, uint32_t scIndex) override;
  ///@}

  ///@{
  /**
   * Return the list of GL formats supported by vtkOpenGLRenderWindow.
   * The first in the list that is also supported by the runtime is picked.
   */
  const std::vector<int64_t>& GetSupportedColorFormats() override;
  const std::vector<int64_t>& GetSupportedDepthFormats() override;
  ///@}

  /**
   * Fill the pointer to the XrGraphicsBindingOpenGL structure.
   */
  bool CreateGraphicsBinding(vtkOpenGLRenderWindow* helperWindow) override;

  /**
   * Return pointer to the XrGraphicsBindingOpenGL structure required to create the OpenXR session.
   */
  const void* GetGraphicsBinding() override { return this->GraphicsBinding.get(); };

  /**
   * Check OpenGL version supported by the runtime
   */
  bool CheckGraphicsRequirements(XrInstance instance, XrSystemId id) override;

  /**
   * Return the extension name corresponding to the OpenGL rendering backend
   */
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

  std::shared_ptr<XrGraphicsBindingOpenGL> GraphicsBinding;

private:
  vtkOpenXRManagerOpenGLGraphics(const vtkOpenXRManagerOpenGLGraphics&) = delete;
  void operator=(const vtkOpenXRManagerOpenGLGraphics&) = delete;

  class PIMPL;
  PIMPL* Private;
};

VTK_ABI_NAMESPACE_END
#endif
