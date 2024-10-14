// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVRRenderWindow
 * @brief   VR rendering window
 *
 *
 * vtkVRRenderWindow is a abstract class to define a RenderWindow in a
 * VR context.
 *
 * VR provides HMD and controller positions in the "Physical" coordinate
 * system. For room scale VR this is based on the room setup
 * per the underlying VR API being used. Units are in meters.
 *
 * Renderer shows actors in the World coordinate system. Transformation between
 * Physical and World coordinate systems is defined by PhysicalToWorldMatrix.
 * This matrix determines the user's position and orientation in the rendered
 * scene and scaling (magnification) of rendered actors.
 *
 * This class introduces the notion of DeviceHandles. A DeviceHandle is
 * a uint32_t handle that represents a device in the underlying VR API
 * such as OpenVR or OpenXR. Implementations of this class are responsible
 * for mapping the actual devices from that API into unique DeviceHandles.
 * Typically these devices are handheld controllers, HMDs, stylus etc.
 * The subclass should call AddDeviceHandle when it first sees a new device.
 *
 * This class also uses the term Device from vtkEventDataDevice to represent
 * a generic device type that is used in the event handling system. Multiple
 * DeviceHandles could point to the same Device though most often there is a
 * one to one mapping. For example vtkEventDataDevice::LeftController will
 * typically have one or zero DeviceHandles that map to it.
 *
 * Most event processing works with generic devices and this class provides a
 * number of methods to support that. This class also provides a number of methods
 * that work on DeviceHandles which are typically used by VR classes and their
 * subclasses.
 */

#ifndef vtkVRRenderWindow_h
#define vtkVRRenderWindow_h

#include "vtkDeprecation.h" // for deprecation
#include "vtkEventData.h"   // for enums
#include "vtkNew.h"         // for vtkNew
#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderingVRModule.h" // For export macro
#include "vtkSmartPointer.h"      // for vtkSmartPointer
#include "vtk_glad.h"             // used for methods

#include <vector> // ivars

VTK_ABI_NAMESPACE_BEGIN
class vtkCamera;
class vtkMatrix4x4;
class vtkVRModel;

class VTKRENDERINGVR_EXPORT vtkVRRenderWindow : public vtkOpenGLRenderWindow
{
public:
  enum
  {
    LeftEye = 0,
    RightEye
  };

  vtkTypeMacro(vtkVRRenderWindow, vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create an interactor to control renderers in this window. Must be
   * overridden to instantiate a specific interactor depending on the API
   */
  vtkRenderWindowInteractor* MakeRenderWindowInteractor() override = 0;

  ///@{
  /**
   * Get the frame buffers used for rendering
   */
  GLuint GetLeftResolveBufferId() { return this->FramebufferDescs[LeftEye].ResolveFramebufferId; }
  GLuint GetRightResolveBufferId() { return this->FramebufferDescs[RightEye].ResolveFramebufferId; }
  void GetRenderBufferSize(int& width, int& height)
  {
    width = this->Size[0];
    height = this->Size[1];
  }
  ///@}

  ///@{
  /**
   * Get the VRModel corresponding to the device or device handle.
   */
  vtkVRModel* GetModelForDevice(vtkEventDataDevice idx);
  vtkVRModel* GetModelForDeviceHandle(uint32_t handle);
  ///@}

  ///@{
  /**
   * Set the VRModel corresponding to the device handle
   */
  void SetModelForDeviceHandle(uint32_t handle, vtkVRModel* model);
  ///@}

  ///@{
  /**
   * Get the DeviceToPhysical matrix corresponding to the device or device handle.
   * e.g. 0,0,0,1 pushed through this matrix will give you the location of the
   * device in physical coordinates.
   */
  vtkMatrix4x4* GetDeviceToPhysicalMatrixForDevice(vtkEventDataDevice idx);
  vtkMatrix4x4* GetDeviceToPhysicalMatrixForDeviceHandle(uint32_t handle);
  ///@}

  ///@{
  /*
   * This method gets a device handle for a given device. index is used to
   * disambiguate when there are multiple device handles that map to a
   * device.
   */
  uint32_t GetDeviceHandleForDevice(vtkEventDataDevice dev, uint32_t index = 0);
  ///@}

  ///@{
  /*
   * This method returns how many device handles map to a device.
   */
  uint32_t GetNumberOfDeviceHandlesForDevice(vtkEventDataDevice dev);
  ///@}

  ///@{
  /*
   * This method adds a device handle if not already present. The second
   * signature also sets the device associated with the device handle.
   */
  void AddDeviceHandle(uint32_t handle);
  void AddDeviceHandle(uint32_t handle, vtkEventDataDevice device);
  ///@}

  ///@{
  /*
   * This method gets a device for a given device handle.
   */
  vtkEventDataDevice GetDeviceForDeviceHandle(uint32_t handle);
  ///@}

  /**
   * Store in \p deviceToWorldMatrix the matrix that goes from device coordinates
   * to world coordinates. e.g. if you push 0,0,0,1 through this matrix you will get
   * the location of the device in world coordinates.
   * Return true if the query is valid, else false.
   */
  bool GetDeviceToWorldMatrixForDevice(
    vtkEventDataDevice device, vtkMatrix4x4* deviceToWorldMatrix) override;
  virtual bool GetDeviceToWorldMatrixForDeviceHandle(
    uint32_t handle, vtkMatrix4x4* deviceToWorldMatrix);

  /**
   * Initialize the HMD to World setting and camera settings so that the VR
   * world view most closely matched the view from the provided camera. This
   * method is useful for initializing a VR world from an existing on screen
   * window and camera. The Renderer and its camera must already be created
   * and set when this is called.
   */
  virtual void InitializeViewFromCamera(vtkCamera* cam);

  /**
   * Add a renderer to the list of renderers.
   */
  void AddRenderer(vtkRenderer*) override;

  /**
   * Make this windows OpenGL context the current context.
   */
  void MakeCurrent() override;

  /**
   * Release the current context.
   */
  void ReleaseCurrent() override;

  /**
   * Tells if this window is the current OpenGL context for the calling thread.
   */
  bool IsCurrent() override;

  /**
   * Get report of capabilities for the render window
   */
  const char* ReportCapabilities() override { return "VR System"; }

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  vtkTypeBool IsDirect() override { return 1; }

  /**
   * Check to see if a mouse button has been pressed or mouse wheel activated.
   * All other events are ignored by this method.
   * Maybe should return 1 always?
   */
  vtkTypeBool GetEventPending() override { return 0; }

  /**
   * Get the current size of the screen in pixels.
   */
  int* GetScreenSize() override;

  ///@{
  /**
   * Set the size of the window in screen coordinates in pixels.
   * This resizes the operating system's window and redraws it.
   *
   * If the size has changed, this method will fire
   * vtkCommand::WindowResizeEvent.
   */
  void SetSize(int width, int height) override;
  void SetSize(int a[2]) override { this->SetSize(a[0], a[1]); }
  ///@}

  ///@{
  /**
   * Implement required virtual functions.
   */
  void* GetGenericDisplayId() override { return this->HelperWindow->GetGenericDisplayId(); }
  void* GetGenericWindowId() override { return this->HelperWindow->GetGenericWindowId(); }
  void* GetGenericParentId() override { return nullptr; }
  void* GetGenericContext() override { return this->HelperWindow->GetGenericContext(); }
  void* GetGenericDrawable() override { return this->HelperWindow->GetGenericDrawable(); }
  ///@}

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  int SupportsOpenGL() override { return 1; }

  /**
   * Overridden to not release resources that would interfere with an external
   * application's rendering. Avoiding round trip.
   */
  void Render() override;

  ///@{
  /**
   * Set/Get the window to use for the openGL context
   */
  vtkGetObjectMacro(HelperWindow, vtkOpenGLRenderWindow);
  void SetHelperWindow(vtkOpenGLRenderWindow* val);
  ///@}

  /**
   * Get the state object used to keep track of OpenGL state.
   */
  vtkOpenGLState* GetState() override;

  /**
   * Free up any graphics resources associated with this window
   * a value of nullptr means the context may already be destroyed
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  /**
   * Render the controller and base station models
   */
  virtual void RenderModels() = 0;

  ///@{
  /**
   * When on the camera will track the HMD position.
   * On is the default.
   */
  VTK_DEPRECATED_IN_9_4_0("Please use vtkVRCamera::SetTrackHMD() instead.")
  virtual void SetTrackHMD(bool);
  VTK_DEPRECATED_IN_9_4_0("Please use vtkVRCamera::GetTrackHMD() instead.")
  virtual bool GetTrackHMD() VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Set/Get the visibility of the base stations. Defaults to false
   */
  vtkGetMacro(BaseStationVisibility, bool);
  vtkSetMacro(BaseStationVisibility, bool);
  vtkBooleanMacro(BaseStationVisibility, bool);
  ///@}

  /**
   * Update the HMD pose.
   */
  virtual void UpdateHMDMatrixPose(){};

  /**
   * Get whether the XR components of the window
   * have been initialized successfully.
   */
  vtkGetMacro(VRInitialized, bool);

protected:
  vtkVRRenderWindow();
  ~vtkVRRenderWindow() override;

  struct FramebufferDesc
  {
    GLuint ResolveFramebufferId = 0;
    GLuint ResolveColorTextureId = 0;
    GLuint ResolveDepthTextureId = 0;
  };

  void CreateAWindow() override {}
  void DestroyWindow() override {}

  /**
   * Attempt to get the size of the display from the API
   * and store it in this->Size.
   * If succeed, returns true, else false.
   */
  virtual bool GetSizeFromAPI() = 0;

  virtual std::string GetWindowTitleFromAPI() { return "VTK - VR"; }

  virtual bool CreateFramebuffers(uint32_t viewCount = 2) = 0;
  virtual void RenderFramebuffer(FramebufferDesc& framebufferDesc) = 0;

  bool VRInitialized = false;

  VTK_DEPRECATED_IN_9_4_0("Please use vtkVRCamera::TrackHMD instead.")
  bool TrackHMD = true;

  // One per view (typically one per eye)
  std::vector<FramebufferDesc> FramebufferDescs;

  class DeviceData
  {
  public:
    vtkSmartPointer<vtkVRModel> Model;
    vtkNew<vtkMatrix4x4> DeviceToPhysicalMatrix;
    vtkEventDataDevice Device = vtkEventDataDevice::Unknown;
    uint32_t Index = 0;
  };

  std::map<uint32_t, DeviceData> DeviceHandleToDeviceDataMap;
  uint32_t InvalidDeviceIndex = UINT32_MAX;

  bool BaseStationVisibility = false;

  vtkOpenGLRenderWindow* HelperWindow;

private:
  vtkVRRenderWindow(const vtkVRRenderWindow&) = delete;
  void operator=(const vtkVRRenderWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
