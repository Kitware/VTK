/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenVRRenderWindow - OpenVR rendering window

// .SECTION Description
// vtkOpenVRRenderWindow is a concrete implementation of the abstract
// class vtkRenderWindow. vtkOpenVRRenderer interfaces to the
// OpenVR graphics library

// This class and its similar classes are designed to be drop in
// replacements for VTK. If you link to this module and turn on
// the CMake option VTK_OPENVR_OBJECT_FACTORY, the object
// factory mechanism should replace the core rendering classes such as
// RenderWindow with OpenVR specialized versions. The goal is for VTK
// programs to be able to use the OpenVR library with little to no
// changes.

// This class handles the bulk of interfacing to OpenVR. It supports one
// renderer currently. The renderer is assumed to cover the entire window
// which is what makes sense to VR. Overlay renderers can probably be
// made to work with this but consider how overlays will appear in a
// HMD if they do not track the viewpoint etc. This class is based on
// sample code from the OpenVR project.


#ifndef vtkOpenVRRenderWindow_h
#define vtkOpenVRRenderWindow_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkOpenGLRenderWindow.h"

#define SDL_MAIN_HANDLED
#include <SDL.h> // for ivars
#include <openvr.h> // for ivars
#include <vector> // ivars
#include "vtkOpenGLHelper.h" // used for ivars
#include "vtk_glew.h" // used for methods

class vtkOpenVRModel;
class vtkOpenGLVertexBufferObject;
class vtkTransform;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkOpenVRRenderWindow *New();
  vtkTypeMacro(vtkOpenVRRenderWindow,vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Begin the rendering process.
  virtual void Start(void);

  // Description:
  // Update the system, if needed, due to stereo rendering. For some stereo
  // methods, subclasses might need to switch some hardware settings here.
  virtual void StereoUpdate();

  // Description:
  // Intermediate method performs operations required between the rendering
  // of the left and right eye.
  virtual void StereoMidpoint();

  // Description:
  // Handles work required once both views have been rendered when using
  // stereo rendering.
  virtual void StereoRenderComplete();

  // Description:
  // End the rendering process and display the image.
  void Frame(void);

  // Description:
  // Initialize the rendering window.  This will setup all system-specific
  // resources.  This method and Finalize() must be symmetric and it
  // should be possible to call them multiple times, even changing WindowId
  // in-between.  This is what WindowRemap does.
  virtual void Initialize(void);

  // Description:
  // Finalize the rendering window.  This will shutdown all system-specific
  // resources.  After having called this, it should be possible to destroy
  // a window that was used for a SetWindowId() call without any ill effects.
  virtual void Finalize(void);

  // Description:
  // Make this windows OpenGL context the current context.
  void MakeCurrent();

  // Description:
  // Tells if this window is the current OpenGL context for the calling thread.
  virtual bool IsCurrent();

  // Description:
  // Get report of capabilities for the render window
  const char *ReportCapabilities() { return "OpenVR System";};

  // Description:
  // Is this render window using hardware acceleration? 0-false, 1-true
  int IsDirect() { return 1; };

  // Description:
  // Check to see if a mouse button has been pressed or mouse wheel activated.
  // All other events are ignored by this method.
  // Maybe shoudl return 1 always?
  virtual  int GetEventPending() { return 0;};

  // Description:
  // Clean up device contexts, rendering contexts, etc.
  void Clean();

  // Description:
  // Get the current size of the screen in pixels.
  virtual int *GetScreenSize();

  // Description:
  // Set the size of the window in pixels.
  virtual void SetSize(int,int);
  virtual void SetSize(int a[2]) {vtkOpenGLRenderWindow::SetSize(a);};

    // Description:
  // Set the position of the window.
  virtual void SetPosition(int,int);
  virtual void SetPosition(int a[2]) {vtkOpenGLRenderWindow::SetPosition(a);};

  // implement required virtual functions
  void SetWindowInfo(char *) {};
  void SetNextWindowInfo(char *) {};
  void SetParentInfo(char *) {};
  virtual void *GetGenericDisplayId() {return (void *)this->ContextId;};
  virtual void *GetGenericWindowId()  {return (void *)this->WindowId;};
  virtual void *GetGenericParentId()  {return (void *)NULL;};
  virtual void *GetGenericContext()   {return (void *)this->ContextId;};
  virtual void *GetGenericDrawable()  {return (void *)this->WindowId;};
  virtual void SetDisplayId(void *) {};
  void  SetWindowId(void *) {};
  void  SetParentId(void *) {};
  void HideCursor() {};
  void ShowCursor() {};
  virtual void SetFullScreen(int) {};
  virtual void WindowRemap(void) {};
  virtual void SetNextWindowId(void *) {};

  // Description:
  // Get the system pointer
  vr::IVRSystem *GetHMD() { return this->HMD; };

  // Description:
  // Update the HMD pose
  void UpdateHMDMatrixPose();

  // Description:
  // Does this render window support OpenGL? 0-false, 1-true
  virtual int SupportsOpenGL() { return 1; };

  // Description:
  // Get the frame buffers used for rendering
  GLuint GetLeftRenderBufferId()
    { return this->LeftEyeDesc.m_nRenderFramebufferId; };
  GLuint GetLeftResolveBufferId()
    { return this->LeftEyeDesc.m_nResolveFramebufferId; };
  GLuint GetRightRenderBufferId()
    { return this->RightEyeDesc.m_nRenderFramebufferId; };
  GLuint GetRightResolveBufferId()
    { return this->RightEyeDesc.m_nResolveFramebufferId; };
  void GetRenderBufferSize(int &width, int &height) {
    width = this->RenderWidth; height = this->RenderHeight; };

  // Description:
  // Overridden to not release resources that would interfere with an external
  // application's rendering. Avoiding round trip.
  void Render();

  // Description:
  // Get the most recent pose of any tracked devices
  vr::TrackedDevicePose_t &GetTrackedDevicePose(vr::TrackedDeviceIndex_t idx) {
    return this->TrackedDevicePose[idx]; };

protected:
  vtkOpenVRRenderWindow();
  ~vtkOpenVRRenderWindow();

  // Description:
  // Free up any graphics resources associated with this window
  // a value of NULL means the context may already be destroyed
  virtual void ReleaseGraphicsResources(vtkRenderWindow *);

  virtual void CreateAWindow() {};
  virtual void DestroyWindow() {};

  SDL_Window *WindowId;
  SDL_GLContext ContextId;
  std::string m_strDriver;
  std::string m_strDisplay;
  vr::IVRSystem *HMD;
  vr::IVRRenderModels *OpenVRRenderModels;
  bool VBlank;

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

  // resolution to render to for FBOs
  // (as opposed to the window)
  uint32_t RenderWidth;
  uint32_t RenderHeight;

  // Description:
  // Handle lens distortion
  void SetupDistortion();
  void RenderDistortion();
  vtkOpenGLHelper Distortion;
  vtkOpenGLVertexBufferObject *DistortionVBO;

  // convert a device index to a human string
  std::string GetTrackedDeviceString(
    vr::IVRSystem *pHmd,
    vr::TrackedDeviceIndex_t unDevice,
    vr::TrackedDeviceProperty prop,
    vr::TrackedPropertyError *peError = NULL );

  // devices may have polygonal models
  // load them
  vtkOpenVRModel *FindOrLoadRenderModel(const char *modelName );
  void RenderModels();
  std::vector<vtkOpenVRModel * > VTKRenderModels;
  vtkOpenVRModel *TrackedDeviceToRenderModel[ vr::k_unMaxTrackedDeviceCount ];
  vr::TrackedDevicePose_t TrackedDevicePose[ vr::k_unMaxTrackedDeviceCount ];

  // used in computing the pose
  vtkTransform *HMDTransform;

private:
  vtkOpenVRRenderWindow(const vtkOpenVRRenderWindow&);  // Not implemented.
  void operator=(const vtkOpenVRRenderWindow&);  // Not implemented.
};


#endif
