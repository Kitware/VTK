/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenXRRenderWindow
 * @brief   OpenXR rendering window
 *
 *
 * vtkOpenXRRenderWindow is a concrete implementation of the abstract
 * class vtkRenderWindow.
 *
 * This class and its similar classes are designed to be drop in
 * replacements for VTK. If you link to this module and turn on
 * the CMake option VTK_OPENXR_OBJECT_FACTORY, the object
 * factory mechanism should replace the core rendering classes such as
 * RenderWindow with OpenXR specialized versions. The goal is for VTK
 * programs to be able to use the OpenXR library with little to no
 * changes.
 *
 * This class handles the bulk of interfacing to OpenXR. It supports one
 * renderer currently. The renderer is assumed to cover the entire window
 * which is what makes sense to VR. Overlay renderers can probably be
 * made to work with this but consider how overlays will appear in a
 * HMD if they do not track the viewpoint etc.
 *
 * OpenXR provides HMD and controller positions in "Physical" coordinate
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

#ifndef vtkOpenXRRenderWindow_h
#define vtkOpenXRRenderWindow_h

#include "vtkRenderingOpenXRModule.h" // For export macro
#include "vtkVRRenderWindow.h"

#include "vtkEventData.h" // for method sig
#include "vtkOpenXR.h"

#include <array> // array

class vtkMatrix4x4;

class VTKRENDERINGOPENXR_EXPORT vtkOpenXRRenderWindow : public vtkVRRenderWindow
{
public:
  static vtkOpenXRRenderWindow* New();
  vtkTypeMacro(vtkOpenXRRenderWindow, vtkVRRenderWindow);

  /**
   * Create an interactor to control renderers in this window.
   */
  VTK_NEWINSTANCE vtkRenderWindowInteractor* MakeRenderWindowInteractor() override;

  /**
   * Add a renderer to the list of renderers.
   */
  void AddRenderer(vtkRenderer*) override;

  /**
   * Update the system, if needed, due to stereo rendering. For some stereo
   * methods, subclasses might need to switch some hardware settings here.
   */
  void StereoUpdate() override;

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
   * Overridden to not release resources that would interfere with an external
   * application's rendering. Avoiding round trip.
   */
  void Render() override;

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  void Initialize() override;

  /**
   * Finalize the rendering window.  This will shutdown all system-specific
   * resources.  After having called this, it should be possible to destroy
   * a window that was used for a SetWindowId() call without any ill effects.
   */
  void Finalize() override;

  /**
   * Get report of capabilities for the render window
   */
  const char* ReportCapabilities() override { return "OpenXR System"; }

  /**
   * Get size of render window from OpenXR.
   */
  bool GetSizeFromAPI() override;

  /**
   * Check to see if a mouse button has been pressed or mouse wheel activated.
   * All other events are ignored by this method.
   * Maybe should return 1 always?
   */
  vtkTypeBool GetEventPending() override { return 0; }

  /**
   * Set the active state (active: true / inactive: false) of the specified hand.
   */
  void SetModelActiveState(const int hand, bool state) { this->ModelsActiveState[hand] = state; }

  uint32_t GetDeviceHandleForOpenXRHandle(uint32_t index);
  vtkEventDataDevice GetDeviceForOpenXRHandle(uint32_t ohandle);

  /**
   * Update the HMD pose based on hardware pose and physical to world transform.
   * VR camera properties are directly modified based on physical to world to
   * simulate \sa PhysicalTranslation, \sa PhysicalScale, etc.
   */
  void UpdateHMDMatrixPose() override;

protected:
  vtkOpenXRRenderWindow();
  ~vtkOpenXRRenderWindow() override;

  // Create one framebuffer per view
  bool CreateFramebuffers(uint32_t viewCount = 2) override;

  bool BindTextureToFramebuffer(FramebufferDesc& framebufferDesc);
  void RenderFramebuffer(FramebufferDesc& framebufferDesc);

  void RenderOneEye(const uint32_t eye);

  void RenderModels();

  vtkNew<vtkMatrix4x4> TempMatrix4x4;

  // Store if a model is active or not here as openxr do not have a concept
  // of active/inactive controller
  std::array<bool, 2> ModelsActiveState = { true, true };

private:
  vtkOpenXRRenderWindow(const vtkOpenXRRenderWindow&) = delete;
  void operator=(const vtkOpenXRRenderWindow&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkOpenXRRenderWindow.h
