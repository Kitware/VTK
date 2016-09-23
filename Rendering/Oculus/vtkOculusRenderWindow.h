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
 * @class   vtkOculusRenderWindow
 * @brief   Oculus rendering window
 *
 *
 * vtkOculusRenderWindow is a concrete implementation of the abstract
 * class vtkRenderWindow. vtkOculusRenderer interfaces to the
 * Oculus graphics library
 *
 * This class and its similar classes are designed to be drop in
 * replacements for VTK. If you link to this module and turn on
 * the CMake option VTK_OCULUS_OBJECT_FACTORY, the object
 * factory mechanism should replace the core rendering classes such as
 * RenderWindow with OpenVR specialized versions. The goal is for VTK
 * programs to be able to use the OpenVR library with little to no
 * changes.
 *
 * This class handles the bulk of interfacing to OpenVR. It supports one
 * renderer currently. The renderer is assumed to cover the entire window
 * which is what makes sense to VR. Overlay renderers can probably be
 * made to work with this but consider how overlays will appear in a
 * HMD if they do not track the viewpoint etc.
*/

#ifndef vtkOculusRenderWindow_h
#define vtkOculusRenderWindow_h

#include "vtkRenderingOculusModule.h" // For export macro
#include "vtkOpenGLRenderWindow.h"

// Include the Oculus SDK
#include "OVR_CAPI_GL.h"
#include <SDL.h> // for ivars
#include <vector> // ivars
#include "vtkOpenGLHelper.h" // used for ivars
#include "vtk_glew.h" // used for methods

class vtkOculusModel;
class vtkOpenGLVertexBufferObject;
class vtkTransform;

class VTKRENDERINGOCULUS_EXPORT vtkOculusRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkOculusRenderWindow *New();
  vtkTypeMacro(vtkOculusRenderWindow,vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  const char *ReportCapabilities() { return "Oculus System";};

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  int IsDirect() { return 1; };

  /**
   * Check to see if a mouse button has been pressed or mouse wheel activated.
   * All other events are ignored by this method.
   * Maybe shoudl return 1 always?
   */
  virtual  int GetEventPending() { return 0;};

  /**
   * Clean up device contexts, rendering contexts, etc.
   */
  void Clean();

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

  //@{
  /**
   * Get the system pointer
   */
  ovrSession GetSession() { return this->Session; };
  ovrLayerEyeFov GetOVRLayer() { return this->OVRLayer; };
  //@}

  /**
   * Update the HMD pose
   */
  void UpdateHMDMatrixPose();

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  virtual int SupportsOpenGL() { return 1; };

  //@{
  /**
   * Get the frame buffers used for rendering
   */
  GLuint GetLeftResolveBufferId()
    { return this->LeftEyeDesc.m_nResolveFramebufferId; };
  GLuint GetRightResolveBufferId()
    { return this->RightEyeDesc.m_nResolveFramebufferId; };
  //@}

  /**
   * Overridden to not release resources that would interfere with an external
   * application's rendering. Avoiding round trip.
   */
  void Render();

  ovrVector3f *GetHMDToEyeViewOffsets() {
    return this->HMDToEyeViewOffsets; }

protected:
  vtkOculusRenderWindow();
  ~vtkOculusRenderWindow();

  /**
   * Free up any graphics resources associated with this window
   * a value of NULL means the context may already be destroyed
   */
  virtual void ReleaseGraphicsResources(vtkRenderWindow *);

  virtual void CreateAWindow() {};
  virtual void DestroyWindow() {};

  SDL_Window *WindowId;
  SDL_GLContext ContextId;
  ovrSession Session;
  ovrHmdDesc HMD;
  ovrLayerEyeFov OVRLayer;
  ovrVector3f HMDToEyeViewOffsets[2];

  struct FramebufferDesc
  {
    GLuint m_nDepthBufferId;
    GLuint m_nResolveFramebufferId;
    ovrSizei  RecommendedTexSize;
    ovrTextureSwapChain TextureSwapChain;
  };
  FramebufferDesc LeftEyeDesc;
  FramebufferDesc RightEyeDesc;
  bool CreateFrameBuffer(FramebufferDesc &framebufferDesc );

  // used in computing the pose
  vtkTransform *HMDTransform;

private:
  vtkOculusRenderWindow(const vtkOculusRenderWindow&);  // Not implemented.
  void operator=(const vtkOculusRenderWindow&);  // Not implemented.
};


#endif
