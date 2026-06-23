// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebXRRenderWindow
 * @brief   WebXR rendering window
 *
 *
 * vtkWebXRRenderWindow is a concrete implementation of the abstract
 * class vtkRenderWindow.
 *
 * WebXR provides HMD and controller positions in "Physical" coordinate
 * system.
 * Origin: user's eye position at the time of calibration.
 * Axis directions: x = user's right; y = user's up; z = user's back.
 * Unit: meter.
 *
 * Renderer shows actors in World coordinate system. Transformation between
 * Physical and World coordinate systems is defined by PhysicalToWorldMatrix.
 * This matrix determines the user's position and orientation in the rendered
 * scene and scaling (magnification) of rendered actors.
 *
 */

#ifndef vtkWebXRRenderWindow_h
#define vtkWebXRRenderWindow_h

#include "vtkRenderingWebXRModule.h" // For export macro
#include "vtkVRRenderWindow.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGWEBXR_EXPORT VTK_MARSHALAUTO vtkWebXRRenderWindow : public vtkVRRenderWindow
{
public:
  static vtkWebXRRenderWindow* New();
  vtkTypeMacro(vtkWebXRRenderWindow, vtkVRRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create an interactor to control renderers in this window.
   */
  VTK_NEWINSTANCE vtkRenderWindowInteractor* MakeRenderWindowInteractor() override;

  /**
   * Add a renderer to the list of renderers.
   */
  void AddRenderer(vtkRenderer*) override;

  /**
   * Overridden to not release resources that would interfere with an external
   * application's rendering. Avoiding round trip.
   */
  void Render() override;

  /**
   * Get the size of the color buffer.
   * Returns 0 if not able to determine otherwise sets R G B and A into buffer.
   */
  int GetColorBufferSizes(int* rgba) override;

  /**
   * Specify the selector of the canvas element in the DOM.
   */
  void SetCanvasSelector(const char* _arg);

  /**
   * Get the selector of the canvas element in the DOM.
   */
  char* GetCanvasSelector();

  /**
   * Initialize the rendering window. This will initialize WebXR callbacks
   * for session start, end, error, and render.
   */
  void Initialize() override;

  /**
   * Finalize the rendering window. This will stop the current WebXR session.
   */
  void Finalize() override;

  /**
   * Get report of capabilities for the render window
   */
  const char* ReportCapabilities() override { return "WebXR System"; }

  /**
   * Get size of render window from WebXR.
   */
  bool GetSizeFromAPI() override;

  /**
   * Intermediate method performs operations required between the rendering
   * of the left and right eye.
   */
  void StereoMidpoint() override;

  /**
   * Handles work required once both views have been rendered when using
   * stereo rendering.
   */
  void StereoRenderComplete() override;

  /**
   * Update the HMD pose based on hardware pose and physical to world transform.
   * VR camera properties are directly modified based on physical to world to
   * simulate \sa PhysicalTranslation, \sa PhysicalScale, etc.
   */
  void UpdateHMDMatrixPose() override;

  /**
   * Render the controllers
   */
  void RenderModels() override;

protected:
  vtkWebXRRenderWindow();
  ~vtkWebXRRenderWindow() override;

  /**
   * no-op because the WebXR API already gives us a framebuffer
   */
  bool CreateFramebuffers(uint32_t vtkNotUsed(viewCount) = 1) override { return true; };

  /**
   * no-op to satisfy the base class API
   */
  void RenderFramebuffer(FramebufferDesc& vtkNotUsed(framebufferDesc)) override {};

private:
  vtkWebXRRenderWindow(const vtkWebXRRenderWindow&) = delete;
  void operator=(const vtkWebXRRenderWindow&) = delete;

  vtkNew<vtkMatrix4x4> TempMatrix4x4;

  class vtkInternals;
  vtkInternals* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkWebXRRenderWindow.h
