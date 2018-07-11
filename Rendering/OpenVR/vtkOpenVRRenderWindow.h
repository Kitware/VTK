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
*/

#ifndef vtkOpenVRRenderWindow_h
#define vtkOpenVRRenderWindow_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkOpenGLRenderWindow.h"

#include <openvr.h> // for ivars
#include <vector> // ivars
#include "vtkOpenGLHelper.h" // used for ivars
#include "vtk_glew.h" // used for methods
#include "vtkEventData.h" // for enums

class vtkCamera;
class vtkOpenVRModel;
class vtkOpenVROverlay;
class vtkOpenGLVertexBufferObject;
class vtkTransform;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkOpenVRRenderWindow *New();
  vtkTypeMacro(vtkOpenVRRenderWindow,vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Get the system pointer
   */
  vr::IVRSystem *GetHMD() { return this->HMD; };

  /**
   * Draw the overlay
   */
  void RenderOverlay();

  //@{
  /**
   * Set/Get the overlay to use on the VR dashboard
   */
  vtkGetObjectMacro(DashboardOverlay, vtkOpenVROverlay);
  void SetDashboardOverlay(vtkOpenVROverlay *);
  //@}

  /**
   * Update the HMD pose
   */
  void UpdateHMDMatrixPose();

  //@{
  /**
   * Get the frame buffers used for rendering
   */
  GLuint GetLeftRenderBufferId()
    { return this->LeftEyeDesc.m_nRenderFramebufferId; };
  GLuint GetLeftResolveBufferId()
    { return this->LeftEyeDesc.m_nResolveFramebufferId; };
  GLuint GetRightRenderBufferId()
    { return this->RightEyeDesc.m_nRenderFramebufferId; };
  GLuint GetRightResolveBufferId()
    { return this->RightEyeDesc.m_nResolveFramebufferId; };
  void GetRenderBufferSize(int &width, int &height)
    {
    width = this->Size[0];
    height = this->Size[1];
    };
  //@}

  /**
  * Get the VRModel corresponding to the tracked device
  */
  vtkOpenVRModel *GetTrackedDeviceModel(vtkEventDataDevice idx);
  vtkOpenVRModel *GetTrackedDeviceModel(vr::TrackedDeviceIndex_t idx) {
    return this->TrackedDeviceToRenderModel[idx]; };

  /**
  *Get the openVR Render Models
  */
  vr::IVRRenderModels * GetOpenVRRenderModels() {
    return this->OpenVRRenderModels; };

  /**
  * Get the index corresponding to the tracked device
  */
  vr::TrackedDeviceIndex_t GetTrackedDeviceIndexForDevice(vtkEventDataDevice dev);

  /**
  * Get the most recent pose corresponding to the tracked device
  */
  void GetTrackedDevicePose(vtkEventDataDevice idx, vr::TrackedDevicePose_t **pose);
  vr::TrackedDevicePose_t &GetTrackedDevicePose(vr::TrackedDeviceIndex_t idx) {
    return this->TrackedDevicePose[idx]; };

  /**
   * Initialize the Vive to World setting and camera settings so
   * that the VR world view most closely matched the view from
   * the provided camera. This method is useful for initialing
   * a VR world from an existing on screen window and camera.
   * The Renderer and its camera must already be created and
   * set when this is called.
   */
  void InitializeViewFromCamera(vtkCamera *cam);

  //@{
  /**
   * Control the Vive to World transformations. IN
   * some cases users may not want the Y axis to be up
   * and these methods allow them to control it.
   */
  vtkSetVector3Macro(PhysicalViewDirection, double);
  vtkSetVector3Macro(PhysicalViewUp, double);
  vtkGetVector3Macro(PhysicalViewDirection, double);
  vtkGetVector3Macro(PhysicalViewUp, double);
  vtkSetVector3Macro(PhysicalTranslation, double);
  vtkGetVector3Macro(PhysicalTranslation, double);
  vtkSetMacro(PhysicalScale, double);
  vtkGetMacro(PhysicalScale, double);
  //@}

  //@{
  /**
   * When on the camera will track the HMD position.
   * On is the default.
   */
  vtkSetMacro(TrackHMD, bool);
  vtkGetMacro(TrackHMD, bool);
  //@}

  /**
   * Add a renderer to the list of renderers.
   */
  virtual void AddRenderer(vtkRenderer *) override;

  /**
   * Begin the rendering process.
   */
  virtual void Start(void);

  /**
   * Update the system, if needed, due to stereo rendering. For some stereo
   * methods, subclasses might need to switch some hardware settings here.
   */
  virtual void StereoUpdate();

  /**
   * Intermediate method performs operations required between the rendering
   * of the left and right eye.
   */
  virtual void StereoMidpoint();

  /**
   * Handles work required once both views have been rendered when using
   * stereo rendering.
   */
  virtual void StereoRenderComplete();

  /**
   * End the rendering process and display the image.
   */
  void Frame(void);

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  virtual void Initialize(void);

  /**
   * Finalize the rendering window.  This will shutdown all system-specific
   * resources.  After having called this, it should be possible to destroy
   * a window that was used for a SetWindowId() call without any ill effects.
   */
  virtual void Finalize(void);

  /**
   * Make this windows OpenGL context the current context.
   */
  void MakeCurrent();

  /**
   * Tells if this window is the current OpenGL context for the calling thread.
   */
  virtual bool IsCurrent();

  /**
   * Get report of capabilities for the render window
   */
  const char *ReportCapabilities() { return "OpenVR System";};

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  int IsDirect() { return 1; };

  /**
   * Check to see if a mouse button has been pressed or mouse wheel activated.
   * All other events are ignored by this method.
   * Maybe should return 1 always?
   */
  virtual  int GetEventPending() { return 0;};

  /**
   * Get the current size of the screen in pixels.
   */
  virtual int *GetScreenSize();

  //@{
  /**
   * Set the size of the window in pixels.
   */
  virtual void SetSize(int,int);
  virtual void SetSize(int a[2]) {vtkOpenGLRenderWindow::SetSize(a);};
  //@}

    //@{
    /**
     * Set the position of the window.
     */
  virtual void SetPosition(int,int);
  virtual void SetPosition(int a[2]) {vtkOpenGLRenderWindow::SetPosition(a);};
    //@}

  // implement required virtual functions
  void SetWindowInfo(const char *) {};
  void SetNextWindowInfo(const char *) {};
  void SetParentInfo(const char *) {};
  virtual void *GetGenericDisplayId() {return (void *)this->HelperWindow->GetGenericDisplayId();};
  virtual void *GetGenericWindowId()  {return (void *)this->HelperWindow->GetGenericWindowId();};
  virtual void *GetGenericParentId()  {return (void *)nullptr;};
  virtual void *GetGenericContext() { return (void *)this->HelperWindow->GetGenericContext(); };
  virtual void *GetGenericDrawable()  {return (void *)this->HelperWindow->GetGenericDrawable();};
  virtual void SetDisplayId(void *) {};
  void  SetWindowId(void *) {};
  void  SetParentId(void *) {};
  void HideCursor() {};
  void ShowCursor() {};
  virtual void SetFullScreen(int) {};
  virtual void WindowRemap(void) {};
  virtual void SetNextWindowId(void *) {};

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  virtual int SupportsOpenGL() { return 1; };

  /**
   * Overridden to not release resources that would interfere with an external
   * application's rendering. Avoiding round trip.
   */
  void Render();

  /**
   * Set/Get the window to use for the openGL context
   */
  vtkGetObjectMacro(HelperWindow, vtkOpenGLRenderWindow);
  void SetHelperWindow(vtkOpenGLRenderWindow *val);

  // Get the state object used to keep track of
  // OpenGL state
  vtkOpenGLState *GetState() override;

protected:
  vtkOpenVRRenderWindow();
  ~vtkOpenVRRenderWindow();

  /**
   * Free up any graphics resources associated with this window
   * a value of nullptr means the context may already be destroyed
   */
  virtual void ReleaseGraphicsResources(vtkRenderWindow *);

  virtual void CreateAWindow() {};
  virtual void DestroyWindow() {};

  std::string m_strDriver;
  std::string m_strDisplay;
  vr::IVRSystem *HMD;
  vr::IVRRenderModels *OpenVRRenderModels;

  struct FramebufferDesc
  {
    GLuint m_nDepthBufferId;
    GLuint m_nRenderTextureId;
    GLuint m_nRenderFramebufferId;
    GLuint m_nResolveTextureId;
    GLuint m_nResolveFramebufferId;
  };
  FramebufferDesc LeftEyeDesc;
  FramebufferDesc RightEyeDesc;
  bool CreateFrameBuffer( int nWidth, int nHeight,
    FramebufferDesc &framebufferDesc );

  // convert a device index to a human string
  std::string GetTrackedDeviceString(
    vr::IVRSystem *pHmd,
    vr::TrackedDeviceIndex_t unDevice,
    vr::TrackedDeviceProperty prop,
    vr::TrackedPropertyError *peError = nullptr );

  // devices may have polygonal models
  // load them
  vtkOpenVRModel *FindOrLoadRenderModel(const char *modelName );
  void RenderModels();
  std::vector<vtkOpenVRModel * > VTKRenderModels;
  vtkOpenVRModel *TrackedDeviceToRenderModel[ vr::k_unMaxTrackedDeviceCount ];
  vr::TrackedDevicePose_t TrackedDevicePose[ vr::k_unMaxTrackedDeviceCount ];

  // used in computing the pose
  vtkTransform *HMDTransform;
  double PhysicalViewDirection[3];
  double PhysicalViewUp[3];
  double PhysicalTranslation[3];
  double PhysicalScale;

  // for the overlay
  vtkOpenVROverlay *DashboardOverlay;

  bool TrackHMD;

  vtkOpenGLRenderWindow *HelperWindow;


private:
  vtkOpenVRRenderWindow(const vtkOpenVRRenderWindow&) = delete;
  void operator=(const vtkOpenVRRenderWindow&) = delete;
};


#endif
