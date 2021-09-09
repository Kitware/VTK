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
 * @class   vtkOpenVRRenderWindow
 * @brief   OpenVR rendering window
 *
 *
 * vtkOpenVRRenderWindow is a concrete implementation of the abstract
 * class vtkRenderWindow. vtkOpenVRRenderer interfaces to the
 * OpenVR graphics library
 *
 * This class and its similar classes are designed to be drop in
 * replacements for VTK. If you link to this module and turn on
 * the CMake option VTK_OPENVR_OBJECT_FACTORY, the object
 * factory mechanism should replace the core rendering classes such as
 * RenderWindow with OpenVR specialized versions. The goal is for VTK
 * programs to be able to use the OpenVR library with little to no
 * changes.
 *
 * This class handles the bulk of interfacing to OpenVR. It supports one
 * renderer currently. The renderer is assumed to cover the entire window
 * which is what makes sense to VR. Overlay renderers can probably be
 * made to work with this but consider how overlays will appear in a
 * HMD if they do not track the viewpoint etc. This class is based on
 * sample code from the OpenVR project.
 *
 * OpenVR provides HMD and controller positions in "Physical" coordinate
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

#ifndef vtkOpenVRRenderWindow_h
#define vtkOpenVRRenderWindow_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkVRRenderWindow.h"

#include "vtkEventData.h"    // for enums
#include "vtkOpenGLHelper.h" // used for ivars
#include "vtk_glew.h"        // used for methods
#include <openvr.h>          // for ivars
#include <vector>            // ivars

class vtkCamera;
class vtkMatrix4x4;
class vtkOpenVRModel;
class vtkOpenVROverlay;
class vtkOpenGLVertexBufferObject;
class vtkTransform;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRRenderWindow : public vtkVRRenderWindow
{
public:
  static vtkOpenVRRenderWindow* New();
  vtkTypeMacro(vtkOpenVRRenderWindow, vtkVRRenderWindow);

  void Initialize(void) override;

  void ReleaseGraphicsResources(vtkWindow* renWin) override;

  /**
   * Get the system pointer
   */
  vr::IVRSystem* GetHMD() { return this->HMD; }

  static bool IsHMDPresent();

  /**
   * Create an interactor to control renderers in this window.
   * Creates one specific to OpenVR
   */
  vtkRenderWindowInteractor* MakeRenderWindowInteractor() override;

  void Render() override;

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
   * Draw the overlay
   */
  void RenderOverlay();

  ///@{
  /*
   * Set/Get the overlay to use on the VR dashboard
   */
  vtkGetObjectMacro(DashboardOverlay, vtkOpenVROverlay);
  void SetDashboardOverlay(vtkOpenVROverlay*);
  ///@}

  //@{
  /**
   * Set/Get the visibility of the base stations. Defaults to false
   */
  vtkGetMacro(BaseStationVisibility, bool);
  vtkSetMacro(BaseStationVisibility, bool);
  vtkBooleanMacro(BaseStationVisibility, bool);
  //@}

  /**
   * Update the HMD pose based on hardware pose and physical to world transform.
   * VR camera properties are directly modified based on physical to world to
   * simulate \sa PhysicalTranslation, \sa PhysicalScale, etc.
   */
  void UpdateHMDMatrixPose() override;

  using vtkVRRenderWindow::GetTrackedDeviceModel;

  /**
   * Get the VRModel corresponding to the tracked device
   */
  vtkVRModel* GetTrackedDeviceModel(vtkEventDataDevice idx, uint32_t index) override;

  /**
   * Get the openVR Render Models
   */
  vr::IVRRenderModels* GetOpenVRRenderModels() { return this->OpenVRRenderModels; }

  /**
   * Get the EventDataDevice corresponding to the OpenVR index
   */
  vtkEventDataDevice GetDeviceFromDeviceIndex(vr::TrackedDeviceIndex_t index);

  /**
   * Get the index corresponding to the tracked device
   */
  vr::TrackedDeviceIndex_t GetTrackedDeviceIndexForDevice(vtkEventDataDevice dev)
  {
    return this->GetTrackedDeviceIndexForDevice(dev, 0);
  }
  vr::TrackedDeviceIndex_t GetTrackedDeviceIndexForDevice(vtkEventDataDevice dev, uint32_t index);
  uint32_t GetNumberOfTrackedDevicesForDevice(vtkEventDataDevice dev);

  /**
   * Get the most recent pose corresponding to the tracked device
   */
  vr::TrackedDevicePose_t* GetTrackedDevicePose(vtkEventDataDevice idx)
  {
    return this->GetTrackedDevicePose(idx, 0);
  }
  vr::TrackedDevicePose_t* GetTrackedDevicePose(vtkEventDataDevice idx, uint32_t index);
  vr::TrackedDevicePose_t& GetTrackedDevicePose(vr::TrackedDeviceIndex_t idx)
  {
    return this->TrackedDevicePose[idx];
  }

  /**
   * Render the controller and base station models
   */
  void RenderModels() override;

  bool GetPoseMatrixWorldFromDevice(
    vtkEventDataDevice device, vtkMatrix4x4* poseMatrixWorld) override;

  void ConvertOpenVRPoseToMatrices(const vr::TrackedDevicePose_t& tdPose,
    vtkMatrix4x4* poseMatrixWorld, vtkMatrix4x4* poseMatrixPhysical = nullptr);

protected:
  vtkOpenVRRenderWindow();
  ~vtkOpenVRRenderWindow() override;

  std::string GetWindowTitleFromAPI() override;

  bool GetSizeFromAPI() override;

  bool BaseStationVisibility;

  vr::IVRSystem* HMD;
  vr::IVRRenderModels* OpenVRRenderModels;

  bool CreateFramebuffers() override;

  bool CreateOneFramebuffer(int nWidth, int nHeight, FramebufferDesc& framebufferDesc);

  // convert a device index to a human string
  std::string GetTrackedDeviceString(vr::IVRSystem* pHmd, vr::TrackedDeviceIndex_t unDevice,
    vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = nullptr);

  // devices may have polygonal models
  // load them
  vtkOpenVRModel* FindOrLoadRenderModel(const char* modelName);

  vr::TrackedDevicePose_t TrackedDevicePose[vr::k_unMaxTrackedDeviceCount];

  vtkOpenVROverlay* DashboardOverlay;

private:
  vtkOpenVRRenderWindow(const vtkOpenVRRenderWindow&) = delete;
  void operator=(const vtkOpenVRRenderWindow&) = delete;
};

#endif
