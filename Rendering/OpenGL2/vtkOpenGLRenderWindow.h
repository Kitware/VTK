/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLRenderWindow
 * @brief   OpenGL rendering window
 *
 * vtkOpenGLRenderWindow is a concrete implementation of the abstract class
 * vtkRenderWindow. vtkOpenGLRenderer interfaces to the OpenGL graphics
 * library. Application programmers should normally use vtkRenderWindow
 * instead of the OpenGL specific version.
*/

#ifndef vtkOpenGLRenderWindow_h
#define vtkOpenGLRenderWindow_h

#include "vtkRect.h" // for vtkRecti
#include "vtkRenderWindow.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkType.h"                   // for ivar
#include <map>                         // for ivar
#include <set>                         // for ivar
#include <string>                      // for ivar

class vtkIdList;
class vtkOpenGLHardwareSupport;
class vtkOpenGLShaderCache;
class vtkOpenGLVertexBufferObjectCache;
class vtkOpenGLVertexArrayObject;
class vtkShaderProgram;
class vtkStdString;
class vtkTexture;
class vtkTextureObject;
class vtkTextureUnitManager;
class vtkGenericOpenGLResourceFreeCallback;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLRenderWindow : public vtkRenderWindow
{
public:
  vtkTypeMacro(vtkOpenGLRenderWindow, vtkRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * What rendering backend has the user requested
   */
  const char *GetRenderingBackend() VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the maximum number of multisamples
   */
  static void SetGlobalMaximumNumberOfMultiSamples(int val);
  static int  GetGlobalMaximumNumberOfMultiSamples();
  //@}

  /**
   * Update system if needed due to stereo rendering.
   */
  void StereoUpdate() VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the pixel data of an image, transmitted as RGBRGB...
   */
  unsigned char *GetPixelData(int x,int y,int x2,int y2,int front) VTK_OVERRIDE;
  int GetPixelData(int x,int y,int x2,int y2, int front,
                           vtkUnsignedCharArray *data) VTK_OVERRIDE;
  int SetPixelData(int x,int y,int x2,int y2,unsigned char *data,
                           int front) VTK_OVERRIDE;
  int SetPixelData(int x,int y,int x2,int y2,
                           vtkUnsignedCharArray *data, int front) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Set/Get the pixel data of an image, transmitted as RGBARGBA...
   */
  float *GetRGBAPixelData(int x,int y,int x2,int y2,int front) VTK_OVERRIDE;
  int GetRGBAPixelData(int x,int y,int x2,int y2, int front,
                               vtkFloatArray* data) VTK_OVERRIDE;
  int SetRGBAPixelData(int x,int y,int x2,int y2, float *data,
                               int front, int blend=0) VTK_OVERRIDE;
  int SetRGBAPixelData(int x,int y,int x2,int y2, vtkFloatArray *data,
                               int front, int blend=0) VTK_OVERRIDE;
  void ReleaseRGBAPixelData(float *data) VTK_OVERRIDE;
  unsigned char *GetRGBACharPixelData(int x,int y,int x2,int y2,
                                              int front) VTK_OVERRIDE;
  int GetRGBACharPixelData(int x,int y,int x2,int y2, int front,
                                   vtkUnsignedCharArray *data) VTK_OVERRIDE;
  int SetRGBACharPixelData(int x, int y, int x2, int y2,
                                   unsigned char *data, int front,
                                   int blend=0) VTK_OVERRIDE;
  int SetRGBACharPixelData(int x,int y,int x2,int y2,
                                   vtkUnsignedCharArray *data, int front,
                                   int blend=0) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Set/Get the zbuffer data from an image
   */
  float *GetZbufferData( int x1, int y1, int x2, int y2 ) VTK_OVERRIDE;
  int GetZbufferData( int x1, int y1, int x2, int y2, float* z ) VTK_OVERRIDE;
  int GetZbufferData( int x1, int y1, int x2, int y2,
                              vtkFloatArray* z ) VTK_OVERRIDE;
  int SetZbufferData( int x1, int y1, int x2, int y2, float *buffer ) VTK_OVERRIDE;
  int SetZbufferData( int x1, int y1, int x2, int y2,
                              vtkFloatArray *buffer ) VTK_OVERRIDE;
  //@}


  /**
   * Activate a texture unit for this texture
   */
  void ActivateTexture(vtkTextureObject *);

  /**
   * Deactive a previously activated texture
   */
  void DeactivateTexture(vtkTextureObject *);

  /**
   * Get the texture unit for a given texture object
   */
  int GetTextureUnitForTexture(vtkTextureObject *);

  /**
   * Get the size of the depth buffer.
   */
  int GetDepthBufferSize() VTK_OVERRIDE;

  /**
   * Get the size of the color buffer.
   * Returns 0 if not able to determine otherwise sets R G B and A into buffer.
   */
  int GetColorBufferSizes(int *rgba) VTK_OVERRIDE;

  //@{
  /**
   * Set the size of the window in screen coordinates in pixels.
   */
  void SetSize(int a[2]) VTK_OVERRIDE;
  void SetSize(int,int) VTK_OVERRIDE;
  //@}

  /**
   * Initialize OpenGL for this window.
   */
  virtual void OpenGLInit();

  // Initialize the state of OpenGL that VTK wants for this window
  virtual void OpenGLInitState();

  // Initialize VTK for rendering in a new OpenGL context
  virtual void OpenGLInitContext();

  //@{
  /**
   * Get if the context includes opengl core profile 3.2 support
   */
  static bool GetContextSupportsOpenGL32();
  void SetContextSupportsOpenGL32(bool val);
  //@}

  /**
   * Get the major and minor version numbers of the OpenGL context we are using
   * ala 3.2, 3.3, 4.0, etc... returns 0,0 if opengl has not been initialized
   * yet
   */
  void GetOpenGLVersion(int &major, int &minor);

  /**
   * Return the OpenGL name of the back left buffer.
   * It is GL_BACK_LEFT if GL is bound to the window-system-provided
   * framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
   * application-created framebuffer object (GPU-based offscreen rendering)
   * It is used by vtkOpenGLCamera.
   */
  unsigned int GetBackLeftBuffer();

  /**
   * Return the OpenGL name of the back right buffer.
   * It is GL_BACK_RIGHT if GL is bound to the window-system-provided
   * framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT+1 if GL is bound to an
   * application-created framebuffer object (GPU-based offscreen rendering)
   * It is used by vtkOpenGLCamera.
   */
  unsigned int GetBackRightBuffer();

  /**
   * Return the OpenGL name of the front left buffer.
   * It is GL_FRONT_LEFT if GL is bound to the window-system-provided
   * framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
   * application-created framebuffer object (GPU-based offscreen rendering)
   * It is used by vtkOpenGLCamera.
   */
  unsigned int GetFrontLeftBuffer();

  /**
   * Return the OpenGL name of the front right buffer.
   * It is GL_FRONT_RIGHT if GL is bound to the window-system-provided
   * framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT+1 if GL is bound to an
   * application-created framebuffer object (GPU-based offscreen rendering)
   * It is used by vtkOpenGLCamera.
   */
  unsigned int GetFrontRightBuffer();

  /**
   * Return the OpenGL name of the back left buffer.
   * It is GL_BACK if GL is bound to the window-system-provided
   * framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
   * application-created framebuffer object (GPU-based offscreen rendering)
   * It is used by vtkOpenGLCamera.
   */
  unsigned int GetBackBuffer();

  /**
   * Return the OpenGL name of the front left buffer.
   * It is GL_FRONT if GL is bound to the window-system-provided
   * framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
   * application-created framebuffer object (GPU-based offscreen rendering)
   * It is used by vtkOpenGLCamera.
   */
  unsigned int GetFrontBuffer();

  /**
   * Get the time when the OpenGL context was created.
   */
  virtual vtkMTimeType GetContextCreationTime();

  //@{
  /**
   * Returns an Shader Cache object
   */
  vtkGetObjectMacro(ShaderCache,vtkOpenGLShaderCache);
  //@}

  //@{
  /**
   * Returns an Shader Cache object
   */
  vtkGetObjectMacro(VBOCache,vtkOpenGLVertexBufferObjectCache);
  //@}

  //@{
  /**
   * Returns the current default FBO (0 when OffScreenRendering is inactive).
   */
  vtkGetMacro(FrameBufferObject, unsigned int);
  //@}

  /**
   * Returns its texture unit manager object. A new one will be created if one
   * hasn't already been set up.
   */
  vtkTextureUnitManager *GetTextureUnitManager();

  /**
   * Block the thread until the actual rendering is finished().
   * Useful for measurement only.
   */
  void WaitForCompletion() VTK_OVERRIDE;

  /**
   * Replacement for the old glDrawPixels function
   */
  virtual void DrawPixels(int x1, int y1, int x2, int y2,
              int numComponents, int dataType, void *data);

  /**
   * Replacement for the old glDrawPixels function, but it allows
   * for scaling the data and using only part of the texture
   */
  virtual void DrawPixels(
    int dstXmin, int dstYmin, int dstXmax, int dstYmax,
    int srcXmin, int srcYmin, int srcXmax, int srcYmax,
    int srcWidth, int srcHeight, int numComponents, int dataType, void *data);

  /**
   * Replacement for the old glDrawPixels function.  This simple version draws all
   * the data to the entire current viewport scaling as needed.
   */
  virtual void DrawPixels(
    int srcWidth, int srcHeight, int numComponents, int dataType, void *data);

  /**
   * Return the largest line width supported by the hardware
   */
  virtual float GetMaximumHardwareLineWidth() {
    return this->MaximumHardwareLineWidth; };

  /**
   * Returns true if driver has an
   * EGL/OpenGL bug that makes vtkChartsCoreCxx-TestChartDoubleColors and other tests to fail
   * because point sprites don't work correctly (gl_PointCoord is undefined) unless
   * glEnable(GL_POINT_SPRITE)
   */
  virtual bool IsPointSpriteBugPresent()
  {
    return 0;
  }

  /**
   * Get a mapping of vtk data types to native texture formats for this window
   * we put this on the RenderWindow so that every texture does not have to
   * build these structures themselves
   */
  int GetDefaultTextureInternalFormat(
    int vtktype, int numComponents,
    bool needInteger, bool needFloat);

  /**
   * Return a message profiding additional details about the
   * results of calling SupportsOpenGL()  This can be used
   * to retrieve more specifics about what failed
   */
  std::string GetOpenGLSupportMessage()
  {
    return this->OpenGLSupportMessage;
  }

  // Create and bind offscreen rendering buffers without destroying the current
  // OpenGL context. This allows to temporary switch to offscreen rendering
  // (ie. to make a screenshot even if the window is hidden).
  // Return if the creation was successful (1) or not (0).
  // Note: This function requires that the device supports OpenGL framebuffer extension.
  // The function has no effect if OffScreenRendering is ON.
  int SetUseOffScreenBuffers(bool offScreen) VTK_OVERRIDE;
  bool GetUseOffScreenBuffers() VTK_OVERRIDE;

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  int SupportsOpenGL() VTK_OVERRIDE;

  /**
   * Get report of capabilities for the render window
   */
  const char *ReportCapabilities() VTK_OVERRIDE;

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  virtual void Initialize(void) {};

  std::set<vtkGenericOpenGLResourceFreeCallback *> Resources;

  void RegisterGraphicsResources(vtkGenericOpenGLResourceFreeCallback *cb) {
    std::set<vtkGenericOpenGLResourceFreeCallback *>::iterator it
     = this->Resources.find(cb);
    if (it == this->Resources.end())
    {
      this->Resources.insert(cb);
    }
  }

  void UnregisterGraphicsResources(vtkGenericOpenGLResourceFreeCallback *cb) {
    std::set<vtkGenericOpenGLResourceFreeCallback *>::iterator it
     = this->Resources.find(cb);
    if (it != this->Resources.end())
    {
      this->Resources.erase(it);
    }
  }

  /**
   * Ability to push and pop this window's context
   * as the current context. The idea being to
   * if needed make this window's context current
   * and when done releasing resources restore
   * the prior context.  The default implementation
   * here is only meant as a backup for subclasses
   * that lack a proper implementation.
   */
  virtual void PushContext() { this->MakeCurrent(); }
  virtual void PopContext() {}

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  bool InitializeFromCurrentContext() VTK_OVERRIDE;

  /**
   * Returns the id for the frame buffer object, if any, used by the render window
   * in which the window does all its rendering. This may be 0, in which case
   * the render window is rendering to the default OpenGL render buffers.
   *
   * @returns the name (or id) of the frame buffer object to render to.
   */
  vtkGetMacro(DefaultFrameBufferId, unsigned int);

protected:
  vtkOpenGLRenderWindow();
  ~vtkOpenGLRenderWindow() VTK_OVERRIDE;

  vtkOpenGLShaderCache *ShaderCache;
  vtkOpenGLVertexBufferObjectCache *VBOCache;

  // used in testing for opengl support
  // in the SupportsOpenGL() method
  bool OpenGLSupportTested;
  int OpenGLSupportResult;
  std::string OpenGLSupportMessage;

  int TextureInternalFormats[VTK_UNICODE_STRING][3][5];
  void InitializeTextureInternalFormats();

  std::map<const vtkTextureObject *, int> TextureResourceIds;

  virtual int ReadPixels(const vtkRecti& rect, int front, int glFormat, int glType, void* data);

  /**
   * Create an offScreen window based on OpenGL framebuffer extension.
   * Return if the creation was successful or not.
   * \pre positive_width: width>0
   * \pre positive_height: height>0
   * \pre not_initialized: !OffScreenUseFrameBuffer
   * \post valid_result: (result==0 || result==1)
   * && (result implies OffScreenUseFrameBuffer)
   */
  int CreateHardwareOffScreenWindow(int width, int height);

  int CreateHardwareOffScreenBuffers(int width, int height, bool bind = false);
  void BindHardwareOffScreenBuffers();

  /**
   * Destroy an offscreen window based on OpenGL framebuffer extension.
   * \pre initialized: OffScreenUseFrameBuffer
   * \post destroyed: !OffScreenUseFrameBuffer
   */
  void DestroyHardwareOffScreenWindow();

  void UnbindHardwareOffScreenBuffers();
  void DestroyHardwareOffScreenBuffers();

  /**
   * Flag telling if a framebuffer-based offscreen is currently in use.
   */
  int OffScreenUseFrameBuffer;

  //@{
  /**
   * Variables used by the framebuffer-based offscreen method.
   */
  int NumberOfFrameBuffers;
  unsigned int TextureObjects[4]; // really GLuint
  unsigned int FrameBufferObject; // really GLuint
  unsigned int DepthRenderBufferObject; // really GLuint
  int HardwareBufferSize[2];
  bool HardwareOffScreenBuffersBind;
  //@}

  /**
   * Create a not-off-screen window.
   */
  virtual void CreateAWindow() = 0;

  /**
   * Destroy a not-off-screen window.
   */
  virtual void DestroyWindow() = 0;

  /**
   * Free up any graphics resources associated with this window
   * a value of NULL means the context may already be destroyed
   */
  virtual void ReleaseGraphicsResources(vtkRenderWindow *);

  /**
   * Set the texture unit manager.
   */
  void SetTextureUnitManager(vtkTextureUnitManager *textureUnitManager);


  /**
   * Query and save OpenGL state
   */
  void SaveGLState();

  /**
   * Restore OpenGL state at end of the rendering
   */
  void RestoreGLState();

  std::map<std::string, int> GLStateIntegers;

  unsigned int BackLeftBuffer;
  unsigned int BackRightBuffer;
  unsigned int FrontLeftBuffer;
  unsigned int FrontRightBuffer;
  unsigned int FrontBuffer;
  unsigned int BackBuffer;
  unsigned int DefaultFrameBufferId;

  #ifndef VTK_LEGACY_REMOVE
  /**
   * @deprecated Replaced by
   * vtkOpenGLCheckErrorMacro
   */
  unsigned int LastGraphicError;
  #endif

  /**
   * Flag telling if the context has been created here or was inherited.
   */
  int OwnContext;

  vtkTimeStamp ContextCreationTime;

  vtkTextureUnitManager *TextureUnitManager;

  vtkTextureObject *DrawPixelsTextureObject;

  bool Initialized; // ensure glewinit has been called
  bool GlewInitValid; // Did glewInit initialize with a valid state?

  float MaximumHardwareLineWidth;

  char *Capabilities;

private:
  vtkOpenGLRenderWindow(const vtkOpenGLRenderWindow&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLRenderWindow&) VTK_DELETE_FUNCTION;
};

#endif
