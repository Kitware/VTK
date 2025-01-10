// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO
#include <map>                         // for ivar
#include <set>                         // for ivar
#include <string>                      // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkOpenGLBufferObject;
class vtkOpenGLFramebufferObject;
class vtkOpenGLHardwareSupport;
class vtkOpenGLQuadHelper;
class vtkOpenGLShaderCache;
class vtkOpenGLVertexBufferObjectCache;
class vtkOpenGLVertexArrayObject;
class vtkShaderProgram;
class vtkTexture;
class vtkTextureObject;
class vtkTextureUnitManager;
class vtkGenericOpenGLResourceFreeCallback;
class vtkOpenGLState;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLRenderWindow : public vtkRenderWindow
{
public:
  vtkTypeMacro(vtkOpenGLRenderWindow, vtkRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;
#if !(defined(__APPLE__) || defined(__ANDROID__) || defined(__EMSCRIPTEN__))
  static vtkOpenGLRenderWindow* New();
#endif

  /**
   * Begin the rendering process.
   */
  void Start() override;

  /**
   * A termination method performed at the end of the rendering process
   * to do things like swapping buffers (if necessary) or similar actions.
   */
  void Frame() override;

  /**
   * What rendering backend has the user requested
   */
  const char* GetRenderingBackend() override;

  ///@{
  /**
   * Set/Get the maximum number of multisamples
   */
  static void SetGlobalMaximumNumberOfMultiSamples(int val);
  static int GetGlobalMaximumNumberOfMultiSamples();
  ///@}

  ///@{
  /**
   * Set/Get the pixel data of an image, transmitted as RGBRGB...
   * front in this context indicates that the read should come from the
   * display buffer versus the render buffer
   */
  unsigned char* GetPixelData(int x, int y, int x2, int y2, int front, int right) override;
  int GetPixelData(
    int x, int y, int x2, int y2, int front, vtkUnsignedCharArray* data, int right) override;
  int SetPixelData(
    int x, int y, int x2, int y2, unsigned char* data, int front, int right) override;
  int SetPixelData(
    int x, int y, int x2, int y2, vtkUnsignedCharArray* data, int front, int right) override;
  ///@}

  ///@{
  /**
   * Set/Get the pixel data of an image, transmitted as RGBARGBA...
   */
  float* GetRGBAPixelData(int x, int y, int x2, int y2, int front, int right = 0) override;
  int GetRGBAPixelData(
    int x, int y, int x2, int y2, int front, vtkFloatArray* data, int right = 0) override;
  int SetRGBAPixelData(
    int x, int y, int x2, int y2, float* data, int front, int blend = 0, int right = 0) override;
  int SetRGBAPixelData(int x, int y, int x2, int y2, vtkFloatArray* data, int front, int blend = 0,
    int right = 0) override;
  void ReleaseRGBAPixelData(float* data) override;
  unsigned char* GetRGBACharPixelData(
    int x, int y, int x2, int y2, int front, int right = 0) override;
  int GetRGBACharPixelData(
    int x, int y, int x2, int y2, int front, vtkUnsignedCharArray* data, int right = 0) override;
  int SetRGBACharPixelData(int x, int y, int x2, int y2, unsigned char* data, int front,
    int blend = 0, int right = 0) override;
  int SetRGBACharPixelData(int x, int y, int x2, int y2, vtkUnsignedCharArray* data, int front,
    int blend = 0, int right = 0) override;
  ///@}

  ///@{
  /**
   * Set/Get the zbuffer data from an image
   */
  float* GetZbufferData(int x1, int y1, int x2, int y2) override;
  int GetZbufferData(int x1, int y1, int x2, int y2, float* z) override;
  int GetZbufferData(int x1, int y1, int x2, int y2, vtkFloatArray* buffer) override;
  int SetZbufferData(int x1, int y1, int x2, int y2, float* buffer) override;
  int SetZbufferData(int x1, int y1, int x2, int y2, vtkFloatArray* buffer) override;
  ///@}

  /**
   * Activate a texture unit for this texture
   */
  void ActivateTexture(vtkTextureObject*);

  /**
   * Deactivate a previously activated texture
   */
  void DeactivateTexture(vtkTextureObject*);

  /**
   * Get the texture unit for a given texture object
   */
  int GetTextureUnitForTexture(vtkTextureObject*);

  /**
   * Get the size of the depth buffer.
   */
  int GetDepthBufferSize() override;

  /**
   * Is this window/fo in sRGB colorspace
   */
  bool GetUsingSRGBColorSpace();

  /**
   * Get the size of the color buffer.
   * Returns 0 if not able to determine otherwise sets R G B and A into buffer.
   */
  int GetColorBufferSizes(int* rgba) override;

  /**
   * Get the internal format of current attached texture or render buffer.
   * attachmentPoint is the index of attachment.
   * Returns 0 if not able to determine.
   */
  int GetColorBufferInternalFormat(int attachmentPoint);

  /**
   * Initialize OpenGL for this window.
   */
  virtual void OpenGLInit();

  // Initialize the state of OpenGL that VTK wants for this window
  virtual void OpenGLInitState();

  // Initialize VTK for rendering in a new OpenGL context
  virtual void OpenGLInitContext();

  /**
   * Get the major and minor version numbers of the OpenGL context we are using
   * ala 3.2, 3.3, 4.0, etc... returns 0,0 if opengl has not been initialized
   * yet
   */
  void GetOpenGLVersion(int& major, int& minor);

  /**
   * Get the time when the OpenGL context was created.
   */
  virtual vtkMTimeType GetContextCreationTime();

  /**
   * Returns an Shader Cache object
   */
  vtkOpenGLShaderCache* GetShaderCache();

  /**
   * Returns the VBO Cache
   */
  vtkOpenGLVertexBufferObjectCache* GetVBOCache();

  ///@{
  /**
   * Returns the render framebuffer object.
   */
  vtkGetObjectMacro(RenderFramebuffer, vtkOpenGLFramebufferObject);
  ///@}

  /**
   * Returns the display framebuffer object.
   */
  vtkGetObjectMacro(DisplayFramebuffer, vtkOpenGLFramebufferObject);

  /**
   * Returns its texture unit manager object. A new one will be created if one
   * hasn't already been set up.
   */
  vtkTextureUnitManager* GetTextureUnitManager();

  /**
   * Block the thread until the actual rendering is finished().
   * Useful for measurement only.
   */
  void WaitForCompletion() override;

  /**
   * Replacement for the old glDrawPixels function
   */
  virtual void DrawPixels(
    int x1, int y1, int x2, int y2, int numComponents, int dataType, void* data);

  /**
   * Replacement for the old glDrawPixels function, but it allows
   * for scaling the data and using only part of the texture
   */
  virtual void DrawPixels(int dstXmin, int dstYmin, int dstXmax, int dstYmax, int srcXmin,
    int srcYmin, int srcXmax, int srcYmax, int srcWidth, int srcHeight, int numComponents,
    int dataType, void* data);

  /**
   * Replacement for the old glDrawPixels function.  This simple version draws all
   * the data to the entire current viewport scaling as needed.
   */
  virtual void DrawPixels(int srcWidth, int srcHeight, int numComponents, int dataType, void* data);

  /**
   * Return the largest line width supported by the hardware
   */
  virtual float GetMaximumHardwareLineWidth() { return this->MaximumHardwareLineWidth; }

  /**
   * Returns true if driver has an
   * EGL/OpenGL bug that makes vtkChartsCoreCxx-TestChartDoubleColors and other tests to fail
   * because point sprites don't work correctly (gl_PointCoord is undefined) unless
   * glEnable(GL_POINT_SPRITE)
   */
  virtual bool IsPointSpriteBugPresent() { return false; }

  /**
   * On `gl_PrimitiveID`, the spec says it is a counter that is incremented after every individual
   * point, line or triangle primitive is processed. Almost all OpenGL implementations increment the
   * counter per input primitive type. However, there seems to be a bug in the OpenGL over Metal
   * used in Apple silicon that overwrites any value that the shader writes into `gl_PrimitiveID`.
   *
   * This method returns true if the OpenGL driver has a bug that causes geometry shaders
   * to ignore writes to the gl_PrimitiveID. It checks the OpenGL vendor string for Apple and Metal.
   */
  bool IsPrimIDBugPresent();

  /**
   * Get a mapping of vtk data types to native texture formats for this window
   * we put this on the RenderWindow so that every texture does not have to
   * build these structures themselves
   */
  int GetDefaultTextureInternalFormat(
    int vtktype, int numComponents, bool needInteger, bool needFloat, bool needSRGB);

  /**
   * Return a message profiding additional details about the
   * results of calling SupportsOpenGL()  This can be used
   * to retrieve more specifics about what failed
   */
  std::string GetOpenGLSupportMessage() { return this->OpenGLSupportMessage; }

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  int SupportsOpenGL() override;

  /**
   * Get report of capabilities for the render window
   */
  const char* ReportCapabilities() override;

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  void Initialize() override {}

  std::set<vtkGenericOpenGLResourceFreeCallback*> Resources;

  void RegisterGraphicsResources(vtkGenericOpenGLResourceFreeCallback* cb)
  {
    std::set<vtkGenericOpenGLResourceFreeCallback*>::iterator it = this->Resources.find(cb);
    if (it == this->Resources.end())
    {
      this->Resources.insert(cb);
    }
  }

  void UnregisterGraphicsResources(vtkGenericOpenGLResourceFreeCallback* cb)
  {
    std::set<vtkGenericOpenGLResourceFreeCallback*>::iterator it = this->Resources.find(cb);
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
  bool InitializeFromCurrentContext() override;

  /**
   * Set the number of vertical syncs required between frames.
   * A value of 0 means swap buffers as quickly as possible
   * regardless of the vertical refresh. A value of 1 means swap
   * buffers in sync with the vertical refresh to eliminate tearing.
   * A value of -1 means use a value of 1 unless we missed a frame
   * in which case swap immediately. Returns true if the call
   * succeeded.
   */
  virtual bool SetSwapControl(int) { return false; }

  // Get the state object used to keep track of
  // OpenGL state
  virtual vtkOpenGLState* GetState() { return this->State; }

  // Get a VBO that can be shared by many
  // It consists of normalized display
  // coordinates for a quad and tcoords
  vtkOpenGLBufferObject* GetTQuad2DVBO();

  // Activate and return thje texture unit for a generic 2d 64x64
  // float greyscale noise texture ranging from 0 to 1. The texture is
  // a hard-coded blue noise texture.  This texture unit will automatically
  // be deactivated at the end of the render process.
  int GetNoiseTextureUnit();

  /**
   * Update the system, if needed, at end of render process
   */
  void End() override;

  /**
   * Handle opengl specific code and calls superclass
   */
  void Render() override;

  /**
   * Intermediate method performs operations required between the rendering
   * of the left and right eye.
   */
  void StereoMidpoint() override;

  // does VTKs framebuffer require resolving for reading pixels
  bool GetBufferNeedsResolving();

  /**
   * Free up any graphics resources associated with this window
   * a value of NULL means the context may already be destroyed
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  /**
   * Blit a display framebuffer into a currently bound draw destination,
   * color only
   */
  void BlitDisplayFramebuffer();

  /**
   * Blit a display framebuffer into a currently bound draw destination,
   * color and depth
   */
  void BlitDisplayFramebufferColorAndDepth();

  /**
   * Blit a display buffer into a currently bound draw destination
   */
  void BlitDisplayFramebuffer(int right, int srcX, int srcY, int srcWidth, int srcHeight, int destX,
    int destY, int destWidth, int destHeight, int bufferMode, int interpolation);

  ///@{
  /**
   * Blit the currently bound read buffer to the renderbuffer. This is useful for
   * taking rendering from an external system and then having VTK draw on top of it.
   */
  void BlitToRenderFramebuffer(bool includeDepth);
  void BlitToRenderFramebuffer(int srcX, int srcY, int srcWidth, int srcHeight, int destX,
    int destY, int destWidth, int destHeight, int bufferMode, int interpolation);
  ///@}

  /**
   * Define how the resulting image should be blitted when at the end of the Frame() call if
   * SwapBuffers is true
   */
  enum FrameBlitModes
  {
    BlitToHardware,         // hardware buffers
    BlitToCurrent,          // currently bound draw framebuffer, without depth buffer
    BlitToCurrentWithDepth, // currently bound draw framebuffer, including depth buffer
    NoBlit                  // no blit, GUI or external code will handle the blit
  };

  ///@{
  /**
   * SetGet how to handle blits at the end of a Frame() call.
   * Only happens when SwapBuffers is true.
   *
   * The “blit” operation means copying something (pixels) from one region
   * in memory to another. This is a common OpenGL operation: check out the
   * glBlitFramebuffer OpenGL method. VTK OpenGL rasterizes 3D/2D geometry
   * into color/depth attachments of an in-memory framebuffer: in the graphics
   * world, this is called offscreen rendering.
   *
   * The FrameBlitMode tells VTK where to copy the offscreen
   * buffer. Either the default hardware framebuffer or the currently bound
   * framebuffer. Here are the available modes:
   * - BlitToHardware: This mode blits to the default framebuffer managed by
   *   platform(Win32/X/Cocoa) render windows. This is the default mode when VTK
   *   is used through one of the platform render windows (with vtkRenderWindow).
   * - BlitToCurrent: This mode blits to the currently bound framebuffer.
   *   It is useful when an external framebuffer is bound just before
   *   the vtkRenderWindow::Frame() call. You’ll need this when integrating
   *   VTK with other UI frameworks because these UI frameworks create/have
   *   their own framebuffers. This only blits the color buffer.
   * - BlitToCurrentWithDepth: Almost the same as BlitToCurrent, but this mode
   *   also blits the depth buffer.
   * - NoBlit: no blit. The GUI or external code will handle the blit.
   */
  vtkSetClampMacro(FrameBlitMode, FrameBlitModes, BlitToHardware, NoBlit);
  vtkGetMacro(FrameBlitMode, FrameBlitModes);
  void SetFrameBlitModeToBlitToHardware() { this->SetFrameBlitMode(BlitToHardware); }
  void SetFrameBlitModeToBlitToCurrent() { this->SetFrameBlitMode(BlitToCurrent); }
  void SetFrameBlitModeToBlitToCurrentWithDepth()
  {
    this->SetFrameBlitMode(BlitToCurrentWithDepth);
  }
  void SetFrameBlitModeToNoBlit() { this->SetFrameBlitMode(NoBlit); }
  ///@}

  ///@{
  /**
   * Enable/Disable flipping the Y axis of the rendered texture.
   */
  vtkSetMacro(FramebufferFlipY, bool);
  vtkGetMacro(FramebufferFlipY, bool);
  vtkBooleanMacro(FramebufferFlipY, bool);
  ///@}

  ///@{
  /**
   * Give a target bit size for depth buffers of created Framebuffer Objects. Default is 32.
   */
  vtkSetMacro(RenderBufferTargetDepthSize, int);
  vtkGetMacro(RenderBufferTargetDepthSize, int);
  ///@}

  ///@{
  // copy depth values from a source framebuffer to a destination framebuffer
  // using texture maps to do the copy. The source framebufferobject must be texture
  // backed. This method is designed to work around issues with trying to blit depth
  // values between framebuffers that have different depth formats.

  // blit entire source texture to active viewport
  virtual void TextureDepthBlit(vtkTextureObject* source);

  // blit specified source texels to active viewport
  virtual void TextureDepthBlit(vtkTextureObject* source, int srcX, int srcY, int srcX2, int srcY2);

  // blit specified source texels to specified viewport
  virtual void TextureDepthBlit(vtkTextureObject* source, int srcX, int srcY, int srcX2, int srcY2,
    int destX, int destY, int destX2, int destY2);
  ///@}

  typedef void (*VTKOpenGLAPIProc)();
  typedef VTKOpenGLAPIProc (*VTKOpenGLLoaderFunction)(void* userptr, const char* name);
  /**
   * Provide a function pointer which can load OpenGL core/extension functions.
   * OpenGL proc loader. This is provided by the window system.
   * Here's a brief listing of possible values.
   * For glx: glXGetProcAddress
   * For egl: eglGetProcAddress
   * For wgl: wglGetProcAddress, if that returned null, implementation will get the function from
   * opengl32.dll (for gl 1.1 funcs) For osmesa: OSMesaGetProcAddress For cocoa: null, uses dlsym
   * directly
   */
  void SetOpenGLSymbolLoader(VTKOpenGLLoaderFunction loader, void* userData);

protected:
  vtkOpenGLRenderWindow();
  ~vtkOpenGLRenderWindow() override;

  // blits the display buffers to the appropriate hardware buffers
  virtual void BlitDisplayFramebuffersToHardware();

  // when frame is called, at the end blit to the hardware buffers
  FrameBlitModes FrameBlitMode;

  // a FSQ we use to resolve MSAA that handles gamma
  vtkOpenGLQuadHelper* ResolveQuad;

  // a FSQ we use to blit depth values
  vtkOpenGLQuadHelper* DepthBlitQuad;

  // a FSQ we use to flip framebuffer texture
  vtkOpenGLQuadHelper* FlipQuad;

  // a FSQ we use to read depth component on platforms with OpenGL ES implementations
  // because `glReadPixels` cannot be used to read GL_DEPTH_COMPONENT
  vtkOpenGLQuadHelper* DepthReadQuad;

  // flip quad helpers Y tcoord
  bool FramebufferFlipY;

  // resolve and flip renderframebuffer as needed
  // when copying to displayframebuffer. Returns
  // true if the color buffer was copied.
  virtual bool ResolveFlipRenderFramebuffer();

  // On GLES, the depth attachment buffer cannot be downloaded from
  // the GPU with `glReadPixels`.
  // This method reads the depth buffer bits.
  // The depth attachment size can be 8,16,24 or 32. The values are split into 4 8-bit numbers.
  // These are stored in the form of an RGBA color attachment in DepthFrameBuffer.
  // `glReadPixels` can read that RGBA format and reconstruct full 8,16,24 or 32-bit integer
  // followed by scaling down to 0-1.
  bool ReadDepthComponent(int depthSize);

  // used in testing for opengl support
  // in the SupportsOpenGL() method
  bool OpenGLSupportTested;
  int OpenGLSupportResult;
  std::string OpenGLSupportMessage;

  virtual int ReadPixels(
    const vtkRecti& rect, int front, int glFormat, int glType, void* data, int right = 0);

  /**
   * Create the offScreen framebuffers or resize them if they are already created.
   * Return if the creation was successful or not.
   * \pre positive_width: width>0
   * \pre positive_height: height>0
   * \pre not_initialized: !OffScreenUseFrameBuffer
   * \post valid_result: (result==0 || result==1)
   */
  int CreateFramebuffers(int width, int height);
  vtkOpenGLFramebufferObject* RenderFramebuffer;
  vtkOpenGLFramebufferObject* DisplayFramebuffer;

  // used when we need to resolve a multisampled
  // framebuffer
  vtkOpenGLFramebufferObject* ResolveFramebuffer;

  // used when we need to read depth component
  // with OpenGL ES 3
  vtkOpenGLFramebufferObject* DepthFramebuffer;

  /**
   * Create a not-off-screen window.
   */
  virtual void CreateAWindow() = 0;

  /**
   * Destroy a not-off-screen window.
   */
  virtual void DestroyWindow() = 0;

  /**
   * Query and save OpenGL state
   */
  void SaveGLState();

  /**
   * Restore OpenGL state at end of the rendering
   */
  void RestoreGLState();

  std::map<std::string, int> GLStateIntegers;

  /**
   * Flag telling if the context has been created here or was inherited.
   */
  vtkTypeBool OwnContext;

  vtkTimeStamp ContextCreationTime;

  vtkTextureObject* DrawPixelsTextureObject;

  float MaximumHardwareLineWidth;

  char* Capabilities;

  // used for fast quad rendering
  vtkOpenGLBufferObject* TQuad2DVBO;

  // noise texture
  vtkTextureObject* NoiseTextureObject;

  double FirstRenderTime;

  // keep track of in case we need to recreate the framebuffer
  int LastMultiSamples;

  // how much bits to use for depth of created Framebuffers
  int RenderBufferTargetDepthSize;

  int ScreenSize[2];

  struct
  {
    VTKOpenGLLoaderFunction LoadFunction = nullptr;
    void* UserData = nullptr;
  } SymbolLoader;

private:
  vtkOpenGLRenderWindow(const vtkOpenGLRenderWindow&) = delete;
  void operator=(const vtkOpenGLRenderWindow&) = delete;

  // Keeping `State` private so the only way to access it is through
  // `this->GetState()`.
  vtkOpenGLState* State;
};

VTK_ABI_NAMESPACE_END
#endif
