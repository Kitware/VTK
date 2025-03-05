// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRenderWindow
 * @brief   create a window for renderers to draw into
 *
 * vtkRenderWindow is an abstract object to specify the behavior of a
 * rendering window. A rendering window is a window in a graphical user
 * interface where renderers draw their images. Methods are provided to
 * synchronize the rendering process, set window size, and control double
 * buffering.  The window also allows rendering in stereo.  The interlaced
 * render stereo type is for output to a VRex stereo projector.  All of the
 * odd horizontal lines are from the left eye, and the even lines are from
 * the right eye.  The user has to make the render window aligned with the
 * VRex projector, or the eye will be swapped.
 *
 * @warning
 * In VTK versions 4 and later, the vtkWindowToImageFilter class is
 * part of the canonical way to output an image of a window to a file
 * (replacing the obsolete SaveImageAsPPM method for vtkRenderWindows
 * that existed in 3.2 and earlier).  Connect one of these filters to
 * the output of the window, and filter's output to a writer such as
 * vtkPNGWriter.
 *
 * @sa
 * vtkRenderer vtkRenderWindowInteractor vtkWindowToImageFilter
 */

#ifndef vtkRenderWindow_h
#define vtkRenderWindow_h

#include "vtkEventData.h"           // for enums
#include "vtkNew.h"                 // For vtkNew
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkSmartPointer.h"        // For vtkSmartPointer
#include "vtkWindow.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkFloatArray;
class vtkProp;
class vtkCollection;
class vtkMatrix4x4;
class vtkRenderTimerLog;
class vtkRenderWindowInteractor;
class vtkRenderer;
class vtkRendererCollection;
class vtkStereoCompositor;
class vtkUnsignedCharArray;

// lets define the different types of stereo
#define VTK_STEREO_CRYSTAL_EYES 1
#define VTK_STEREO_RED_BLUE 2
#define VTK_STEREO_INTERLACED 3
#define VTK_STEREO_LEFT 4
#define VTK_STEREO_RIGHT 5
#define VTK_STEREO_DRESDEN 6
#define VTK_STEREO_ANAGLYPH 7
#define VTK_STEREO_CHECKERBOARD 8
#define VTK_STEREO_SPLITVIEWPORT_HORIZONTAL 9
#define VTK_STEREO_FAKE 10
#define VTK_STEREO_EMULATE 11
#define VTK_STEREO_ZSPACE_INSPIRE 12

#define VTK_CURSOR_DEFAULT 0
#define VTK_CURSOR_ARROW 1
#define VTK_CURSOR_SIZENE 2
#define VTK_CURSOR_SIZENW 3
#define VTK_CURSOR_SIZESW 4
#define VTK_CURSOR_SIZESE 5
#define VTK_CURSOR_SIZENS 6
#define VTK_CURSOR_SIZEWE 7
#define VTK_CURSOR_SIZEALL 8
#define VTK_CURSOR_HAND 9
#define VTK_CURSOR_CROSSHAIR 10
#define VTK_CURSOR_CUSTOM 11

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkRenderWindow : public vtkWindow
{
public:
  vtkTypeMacro(vtkRenderWindow, vtkWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct an instance of  vtkRenderWindow with its screen size
   * set to 300x300, borders turned on, positioned at (0,0), double
   * buffering turned on.
   */
  static vtkRenderWindow* New();

  /**
   * Add a renderer to the list of renderers.
   */
  virtual void AddRenderer(vtkRenderer*);

  /**
   * Remove a renderer from the list of renderers.
   */
  void RemoveRenderer(vtkRenderer*);

  /**
   * Query if a renderer is in the list of renderers.
   */
  vtkTypeBool HasRenderer(vtkRenderer*);

  /**
   * What rendering library has the user requested
   */
  static const char* GetRenderLibrary();

  /**
   * What rendering backend has the user requested
   */
  virtual const char* GetRenderingBackend();

  /**
   * Get the render timer log for this window.
   */
  vtkGetNewMacro(RenderTimer, vtkRenderTimerLog);

  /**
   * Return the collection of renderers in the render window.
   */
  vtkRendererCollection* GetRenderers() { return this->Renderers; }

  /**
   * The GL2PS exporter must handle certain props in a special way (e.g. text).
   * This method performs a render and captures all "GL2PS-special" props in
   * the specified collection. The collection will contain a
   * vtkPropCollection for each vtkRenderer in this->GetRenderers(), each
   * containing the special props rendered by the corresponding renderer.
   */
  void CaptureGL2PSSpecialProps(vtkCollection* specialProps);

  ///@{
  /**
   * Returns true if the render process is capturing text actors.
   */
  vtkGetMacro(CapturingGL2PSSpecialProps, int);
  ///@}

  /**
   * Ask each renderer owned by this RenderWindow to render its image and
   * synchronize this process.
   */
  void Render() override;

  /**
   * Start the rendering process for a frame
   */
  virtual void Start() {}

  /**
   * Update the system, if needed, at end of render process
   */
  virtual void End() {}

  /**
   * Initializes the rendering process.
   * The responsibility to set the Initialized boolean to true is
   * left to the subclass.
   */
  virtual void Initialize() {}

  ///@{
  /**
   * Get/set whether or not the window has been initialized yet.
   */
  vtkGetMacro(Initialized, bool);
  ///@}

  /**
   * Finalize the rendering process.
   */
  virtual void Finalize() {}

  /**
   * A termination method performed at the end of the rendering process
   * to do things like swapping buffers (if necessary) or similar actions.
   */
  virtual void Frame() {}

  /**
   * Block the thread until the actual rendering is finished().
   * Useful for measurement only.
   */
  virtual void WaitForCompletion() {}

  /**
   * Performed at the end of the rendering process to generate image.
   * This is typically done right before swapping buffers.
   */
  virtual void CopyResultFrame();

  /**
   * Create an interactor to control renderers in this window. We need
   * to know what type of interactor to create, because we might be in
   * X Windows or MS Windows.
   */
  virtual vtkRenderWindowInteractor* MakeRenderWindowInteractor();

  ///@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   * Set cursor position in window (note that (0,0) is the lower left
   * corner).
   */
  virtual void HideCursor() {}
  virtual void ShowCursor() {}
  virtual void SetCursorPosition(int, int) {}
  ///@}

  ///@{
  /**
   * Change the shape of the cursor.
   */
  vtkSetMacro(CurrentCursor, int);
  vtkGetMacro(CurrentCursor, int);
  ///@}

  ///@{
  /**
   * Set/Get the full path to the custom cursor.
   * This is used when the current cursor is set to VTK_CURSOR_CUSTOM.
   */
  vtkSetFilePathMacro(CursorFileName);
  vtkGetFilePathMacro(CursorFileName);
  ///@}

  ///@{
  /**
   * Turn on/off rendering full screen window size.
   */
  virtual void SetFullScreen(vtkTypeBool) {}
  vtkGetMacro(FullScreen, vtkTypeBool);
  vtkBooleanMacro(FullScreen, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off window manager borders. Typically, you shouldn't turn the
   * borders off, because that bypasses the window manager and can cause
   * undesirable behavior.
   */
  vtkSetMacro(Borders, vtkTypeBool);
  vtkGetMacro(Borders, vtkTypeBool);
  vtkBooleanMacro(Borders, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get whether windows should be coverable (as opposed to always on
   * top).
   */
  vtkGetMacro(Coverable, vtkTypeBool);
  vtkBooleanMacro(Coverable, vtkTypeBool);
  virtual void SetCoverable(vtkTypeBool coverable);
  ///@}

  ///@{
  /**
   * Prescribe that the window be created in a stereo-capable mode. This
   * method must be called before the window is realized. Default is off.
   */
  vtkGetMacro(StereoCapableWindow, vtkTypeBool);
  vtkBooleanMacro(StereoCapableWindow, vtkTypeBool);
  virtual void SetStereoCapableWindow(vtkTypeBool capable);
  ///@}

  ///@{
  /**
   * Set/Get what type of stereo rendering to use.  CrystalEyes
   * mode uses frame-sequential capabilities available in OpenGL
   * to drive LCD shutter glasses and stereo projectors.  RedBlue
   * mode is a simple type of stereo for use with red-blue glasses.
   * Anaglyph mode is a superset of RedBlue mode, but the color
   * output channels can be configured using the AnaglyphColorMask
   * and the color of the original image can be (somewhat) maintained
   * using AnaglyphColorSaturation;  the default colors for Anaglyph
   * mode is red-cyan.  Interlaced stereo mode produces a composite
   * image where horizontal lines alternate between left and right
   * views.  StereoLeft and StereoRight modes choose one or the other
   * stereo view.  Dresden mode is yet another stereoscopic
   * interleaving. Fake simply causes the window to render twice without
   * actually swapping the camera from left eye to right eye. This is useful in
   * certain applications that want to emulate the rendering passes without
   * actually rendering in stereo mode. Emulate is similar to Fake, except that
   * it does render left and right eye. There is no compositing of the resulting
   * images from the two eyes at the end of each render in this mode, hence the
   * result onscreen will be the right eye.
   */
  vtkGetMacro(StereoType, int);
  void SetStereoType(int);
  void SetStereoTypeToCrystalEyes() { this->SetStereoType(VTK_STEREO_CRYSTAL_EYES); }
  void SetStereoTypeToRedBlue() { this->SetStereoType(VTK_STEREO_RED_BLUE); }
  void SetStereoTypeToInterlaced() { this->SetStereoType(VTK_STEREO_INTERLACED); }
  void SetStereoTypeToLeft() { this->SetStereoType(VTK_STEREO_LEFT); }
  void SetStereoTypeToRight() { this->SetStereoType(VTK_STEREO_RIGHT); }
  void SetStereoTypeToDresden() { this->SetStereoType(VTK_STEREO_DRESDEN); }
  void SetStereoTypeToAnaglyph() { this->SetStereoType(VTK_STEREO_ANAGLYPH); }
  void SetStereoTypeToCheckerboard() { this->SetStereoType(VTK_STEREO_CHECKERBOARD); }
  void SetStereoTypeToSplitViewportHorizontal()
  {
    this->SetStereoType(VTK_STEREO_SPLITVIEWPORT_HORIZONTAL);
  }
  void SetStereoTypeToFake() { this->SetStereoType(VTK_STEREO_FAKE); }
  void SetStereoTypeToEmulate() { this->SetStereoType(VTK_STEREO_EMULATE); }
  ///@}

  ///@{
  /**
   * Returns the stereo type as a string.
   */
  const char* GetStereoTypeAsString();
  static const char* GetStereoTypeAsString(int type);
  ///@}

  ///@{
  /**
   * Turn on/off stereo rendering.
   */
  vtkGetMacro(StereoRender, vtkTypeBool);
  void SetStereoRender(vtkTypeBool stereo);
  vtkBooleanMacro(StereoRender, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the use of alpha bitplanes.
   */
  vtkSetMacro(AlphaBitPlanes, vtkTypeBool);
  vtkGetMacro(AlphaBitPlanes, vtkTypeBool);
  vtkBooleanMacro(AlphaBitPlanes, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off point smoothing. Default is off.
   * This must be applied before the first Render.
   */
  vtkSetMacro(PointSmoothing, vtkTypeBool);
  vtkGetMacro(PointSmoothing, vtkTypeBool);
  vtkBooleanMacro(PointSmoothing, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off line smoothing. Default is off.
   * This must be applied before the first Render.
   */
  vtkSetMacro(LineSmoothing, vtkTypeBool);
  vtkGetMacro(LineSmoothing, vtkTypeBool);
  vtkBooleanMacro(LineSmoothing, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off polygon smoothing. Default is off.
   * This must be applied before the first Render.
   */
  vtkSetMacro(PolygonSmoothing, vtkTypeBool);
  vtkGetMacro(PolygonSmoothing, vtkTypeBool);
  vtkBooleanMacro(PolygonSmoothing, vtkTypeBool);
  ///@}

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

  ///@{
  /**
   * Set/get the anaglyph color saturation factor.  This number ranges from
   * 0.0 to 1.0:  0.0 means that no color from the original object is
   * maintained, 1.0 means all of the color is maintained.  The default
   * value is 0.65.  Too much saturation can produce uncomfortable 3D
   * viewing because anaglyphs also use color to encode 3D.
   */
  vtkSetClampMacro(AnaglyphColorSaturation, float, 0.0f, 1.0f);
  vtkGetMacro(AnaglyphColorSaturation, float);
  ///@}

  ///@{
  /**
   * Set/get the anaglyph color mask values.  These two numbers are bits
   * mask that control which color channels of the original stereo
   * images are used to produce the final anaglyph image.  The first
   * value is the color mask for the left view, the second the mask
   * for the right view.  If a bit in the mask is on for a particular
   * color for a view, that color is passed on to the final view; if
   * it is not set, that channel for that view is ignored.
   * The bits are arranged as r, g, and b, so r = 4, g = 2, and b = 1.
   * By default, the first value (the left view) is set to 4, and the
   * second value is set to 3.  That means that the red output channel
   * comes from the left view, and the green and blue values come from
   * the right view.
   */
  vtkSetVector2Macro(AnaglyphColorMask, int);
  vtkGetVectorMacro(AnaglyphColorMask, int, 2);
  ///@}

  /**
   * Remap the rendering window. This probably only works on UNIX right now.
   * It is useful for changing properties that can't normally be changed
   * once the window is up.
   */
  virtual void WindowRemap() {}

  ///@{
  /**
   * Turn on/off buffer swapping between images.
   */
  vtkSetMacro(SwapBuffers, vtkTypeBool);
  vtkGetMacro(SwapBuffers, vtkTypeBool);
  vtkBooleanMacro(SwapBuffers, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the pixel data of an image, transmitted as RGBRGBRGB. The
   * front argument indicates if the front buffer should be used or the back
   * buffer. It is the caller's responsibility to delete the resulting
   * array. It is very important to realize that the memory in this array
   * is organized from the bottom of the window to the top. The origin
   * of the screen is in the lower left corner. The y axis increases as
   * you go up the screen. So the storage of pixels is from left to right
   * and from bottom to top.
   * (x,y) is any corner of the rectangle. (x2,y2) is its opposite corner on
   * the diagonal.
   */
  virtual int SetPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, unsigned char* /*data*/,
    int /*front*/, int /*right*/ = 0)
  {
    return 0;
  }
  virtual int SetPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/,
    vtkUnsignedCharArray* /*data*/, int /*front*/, int /*right*/ = 0)
  {
    return 0;
  }
  ///@}

  ///@{
  /**
   * Same as Get/SetPixelData except that the image also contains an alpha
   * component. The image is transmitted as RGBARGBARGBA... each of which is a
   * float value. The "blend" parameter controls whether the SetRGBAPixelData
   * method blends the data with the previous contents of the frame buffer
   * or completely replaces the frame buffer data.
   */
  virtual float* GetRGBAPixelData(
    int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, int /*front*/, int /*right*/ = 0)
  {
    return nullptr;
  }
  virtual int GetRGBAPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, int /*front*/,
    vtkFloatArray* /*data*/, int /*right*/ = 0)
  {
    return 0;
  }
  virtual int SetRGBAPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, float*, int /*front*/,
    int /*blend*/ = 0, int /*right*/ = 0)
  {
    return 0;
  }
  virtual int SetRGBAPixelData(
    int, int, int, int, vtkFloatArray*, int, int /*blend*/ = 0, int /*right*/ = 0)
  {
    return 0;
  }
  virtual void ReleaseRGBAPixelData(float* /*data*/) {}
  virtual unsigned char* GetRGBACharPixelData(
    int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, int /*front*/, int /*right*/ = 0)
  {
    return nullptr;
  }
  virtual int GetRGBACharPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, int /*front*/,
    vtkUnsignedCharArray* /*data*/, int /*right*/ = 0)
  {
    return 0;
  }
  virtual int SetRGBACharPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/,
    unsigned char* /*data*/, int /*front*/, int /*blend*/ = 0, int /*right*/ = 0)
  {
    return 0;
  }
  virtual int SetRGBACharPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/,
    vtkUnsignedCharArray* /*data*/, int /*front*/, int /*blend*/ = 0, int /*right*/ = 0)
  {
    return 0;
  }
  ///@}

  ///@{
  /**
   * Set/Get the zbuffer data from the frame buffer.
   * (x,y) is any corner of the rectangle. (x2,y2) is its opposite corner on
   * the diagonal.
   */
  virtual float* GetZbufferData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/) { return nullptr; }
  virtual int GetZbufferData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, float* /*z*/)
  {
    return 0;
  }
  virtual int GetZbufferData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, vtkFloatArray* /*z*/)
  {
    return 0;
  }
  virtual int SetZbufferData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, float* /*z*/)
  {
    return 0;
  }
  virtual int SetZbufferData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, vtkFloatArray* /*z*/)
  {
    return 0;
  }
  float GetZbufferDataAtPoint(int x, int y)
  {
    float value = 1.0f;
    this->GetZbufferData(x, y, x, y, &value);
    return value;
  }
  ///@}

  ///@{
  /**
   * This flag is set if the window hasn't rendered since it was created
   */
  vtkGetMacro(NeverRendered, int);
  ///@}

  ///@{
  /**
   * This is a flag that can be set to interrupt a rendering that is in
   * progress.
   */
  vtkGetMacro(AbortRender, int);
  vtkSetMacro(AbortRender, int);
  vtkGetMacro(InAbortCheck, int);
  vtkSetMacro(InAbortCheck, int);
  virtual int CheckAbortStatus();
  ///@}

  /**
   * Check to see if a mouse button has been pressed.  All other events
   * are ignored by this method.  Ideally, you want to abort the render
   * on any event which causes the DesiredUpdateRate to switch from
   * a high-quality rate to a more interactive rate.
   */
  virtual vtkTypeBool GetEventPending() { return 0; }

  /**
   * Are we rendering at the moment
   */
  virtual int CheckInRenderStatus() { return this->InRender; }

  /**
   * Clear status (after an exception was thrown for example)
   */
  virtual void ClearInRenderStatus() { this->InRender = 0; }

  ///@{
  /**
   * Set/Get the desired update rate. This is used with
   * the vtkLODActor class. When using level of detail actors you
   * need to specify what update rate you require. The LODActors then
   * will pick the correct resolution to meet your desired update rate
   * in frames per second. A value of zero indicates that they can use
   * all the time they want to.
   */
  virtual void SetDesiredUpdateRate(double);
  vtkGetMacro(DesiredUpdateRate, double);
  ///@}

  ///@{
  /**
   * Get the number of layers for renderers.  Each renderer should have
   * its layer set individually.  Some algorithms iterate through all layers,
   * so it is not wise to set the number of layers to be exorbitantly large
   * (say bigger than 100).
   */
  vtkGetMacro(NumberOfLayers, int);
  vtkSetClampMacro(NumberOfLayers, int, 1, VTK_INT_MAX);
  ///@}

  ///@{
  /**
   * Get the interactor associated with this render window
   */
  vtkGetObjectMacro(Interactor, vtkRenderWindowInteractor);
  ///@}

  /**
   * Set the interactor to the render window
   */
  virtual void SetInteractor(vtkRenderWindowInteractor*);

  /**
   * This Method detects loops of RenderWindow<->Interactor,
   * so objects are freed properly.
   */
  void UnRegister(vtkObjectBase* o) override;

  ///@{
  /**
   * Dummy stubs for vtkWindow API.
   */
  void SetDisplayId(void*) override {}
  void SetWindowId(void*) override {}
  virtual void SetNextWindowId(void*) {}
  void SetParentId(void*) override {}
  void* GetGenericDisplayId() override { return nullptr; }
  void* GetGenericWindowId() override { return nullptr; }
  void* GetGenericParentId() override { return nullptr; }
  void* GetGenericContext() override { return nullptr; }
  void* GetGenericDrawable() override { return nullptr; }
  void SetWindowInfo(const char*) override {}
  virtual void SetNextWindowInfo(const char*) {}
  void SetParentInfo(const char*) override {}
  ///@}

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  virtual bool InitializeFromCurrentContext() { return false; }

  ///@{
  /**
   * Set/Get an already existing window that this window should
   * share data with if possible. This must be set
   * after the shared render window has been created and initialized
   * but before this window has been initialized. Not all platforms
   * support data sharing.
   */
  virtual void SetSharedRenderWindow(vtkRenderWindow*);
  vtkGetObjectMacro(SharedRenderWindow, vtkRenderWindow);
  virtual bool GetPlatformSupportsRenderWindowSharing() { return false; }
  ///@}

  /**
   * Tells if this window is the current graphics context for the calling
   * thread.
   */
  virtual bool IsCurrent() { return false; }

  /**
   * If called, allow MakeCurrent() to skip cache-check when called.
   * MakeCurrent() reverts to original behavior of cache-checking
   * on the next render.
   */
  virtual void SetForceMakeCurrent() {}

  /**
   * Get report of capabilities for the render window
   */
  virtual const char* ReportCapabilities() { return "Not Implemented"; }

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  virtual int SupportsOpenGL() { return 0; }

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  virtual vtkTypeBool IsDirect() { return 0; }

  /**
   * This method should be defined by the subclass. How many bits of
   * precision are there in the zbuffer?
   */
  virtual int GetDepthBufferSize() { return 0; }

  /**
   * Get the size of the color buffer.
   * Returns 0 if not able to determine otherwise sets R G B and A into buffer.
   */
  virtual int GetColorBufferSizes(int* /*rgba*/) { return 0; }

  ///@{
  /**
   * Set / Get the number of multisamples to use for hardware antialiasing.
   * A value of 1 will be set to 0.
   * Related to OpenGL parameter GL_MAX_SAMPLES.
   */
  virtual void SetMultiSamples(int);
  vtkGetMacro(MultiSamples, int);
  ///@}

  ///@{
  /**
   * Set / Get the availability of the stencil buffer.
   */
  vtkSetMacro(StencilCapable, vtkTypeBool);
  vtkGetMacro(StencilCapable, vtkTypeBool);
  vtkBooleanMacro(StencilCapable, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If there are several graphics card installed on a system,
   * this index can be used to specify which card you want to render to.
   * the default is 0. This may not work on all derived render window and
   * it may need to be set before the first render.
   */
  vtkSetMacro(DeviceIndex, int);
  vtkGetMacro(DeviceIndex, int);
  ///@}
  /**
   * Returns the number of devices (graphics cards) on a system.
   * This may not work on all derived render windows.
   */
  virtual int GetNumberOfDevices() { return 0; }

  ///@{
  /**
   * Set/Get if we want this window to use the sRGB color space.
   * Some hardware/drivers do not fully support this.
   */
  vtkGetMacro(UseSRGBColorSpace, bool);
  vtkSetMacro(UseSRGBColorSpace, bool);
  vtkBooleanMacro(UseSRGBColorSpace, bool);
  ///@}

  enum
  {
    PhysicalToWorldMatrixModified = vtkCommand::UserEvent + 200
  };

  ///@{
  /**
   * Set/get physical coordinate system in world coordinate system.
   *
   * View direction is the -Z axis of the physical coordinate system
   * in world coordinate system.
   * \sa SetPhysicalViewUp, \sa SetPhysicalTranslation,
   * \sa SetPhysicalScale, \sa SetPhysicalToWorldMatrix
   */
  virtual void SetPhysicalViewDirection(double, double, double);
  virtual void SetPhysicalViewDirection(double[3]);
  vtkGetVector3Macro(PhysicalViewDirection, double);
  ///@}

  ///@{
  /**
   * Set/get physical coordinate system in world coordinate system.
   *
   * View up is the +Y axis of the physical coordinate system
   * in world coordinate system.
   * \sa SetPhysicalViewDirection, \sa SetPhysicalTranslation,
   * \sa SetPhysicalScale, \sa SetPhysicalToWorldMatrix
   */
  virtual void SetPhysicalViewUp(double, double, double);
  virtual void SetPhysicalViewUp(double[3]);
  vtkGetVector3Macro(PhysicalViewUp, double);
  ///@}

  ///@{
  /**
   * Set/get physical coordinate system in world coordinate system.
   *
   * Position of the physical coordinate system origin
   * in world coordinates.
   * \sa SetPhysicalViewDirection, \sa SetPhysicalViewUp,
   * \sa SetPhysicalScale, \sa SetPhysicalToWorldMatrix
   */
  virtual void SetPhysicalTranslation(double, double, double);
  virtual void SetPhysicalTranslation(double[3]);
  vtkGetVector3Macro(PhysicalTranslation, double);
  ///@}

  ///@{
  /**
   * Set/get physical coordinate system in world coordinate system.
   *
   * Ratio of distance in world coordinate and physical and system
   * (PhysicalScale = distance_World / distance_Physical).
   * Example: if world coordinate system is in mm then
   * PhysicalScale = 1000.0 makes objects appear in real size.
   * PhysicalScale = 100.0 makes objects appear 10x larger than real size.
   */
  virtual void SetPhysicalScale(double);
  vtkGetMacro(PhysicalScale, double);
  ///@}

  /**
   * Set physical to world transform matrix. Members calculated and set from the matrix:
   * \sa PhysicalViewDirection, \sa PhysicalViewUp, \sa PhysicalTranslation, \sa PhysicalScale
   * The x axis scale is used for \sa PhysicalScale
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  virtual void SetPhysicalToWorldMatrix(vtkMatrix4x4* matrix);

  /**
   * Get physical to world transform matrix. Members used to calculate the matrix:
   * \sa PhysicalViewDirection, \sa PhysicalViewUp, \sa PhysicalTranslation, \sa PhysicalScale
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  virtual void GetPhysicalToWorldMatrix(vtkMatrix4x4* matrix);

  /**
   * Store in \p deviceToWorldMatrix the matrix that goes from device coordinates
   * to world coordinates. e.g. if you push 0,0,0,1 through this matrix you will get
   * the location of the device in world coordinates.
   * Return true if the query is valid, else false.
   */
  virtual bool GetDeviceToWorldMatrixForDevice(
    vtkEventDataDevice device, vtkMatrix4x4* deviceToWorldMatrix);

  ///@{
  /**
   * Set/Get if we want this window to use a translucent surface with alpha channel support.
   * Note that some implementations do not support this.
   * Must be set before window initialization.
   * Default is false.
   */
  vtkGetMacro(EnableTranslucentSurface, bool);
  vtkSetMacro(EnableTranslucentSurface, bool);
  vtkBooleanMacro(EnableTranslucentSurface, bool);
  ///@}

protected:
  vtkRenderWindow();
  ~vtkRenderWindow() override;

  virtual void DoStereoRender();

  vtkRendererCollection* Renderers;
  vtkNew<vtkRenderTimerLog> RenderTimer;
  vtkTypeBool Borders;
  vtkTypeBool Coverable;
  vtkTypeBool FullScreen;
  int OldScreen[5];
  vtkTypeBool PointSmoothing;
  vtkTypeBool LineSmoothing;
  vtkTypeBool PolygonSmoothing;
  vtkTypeBool StereoRender;
  int StereoType;
  vtkTypeBool StereoCapableWindow;
  vtkTypeBool AlphaBitPlanes;
  vtkRenderWindowInteractor* Interactor;
  vtkSmartPointer<vtkUnsignedCharArray> StereoBuffer; // used for red blue stereo
  vtkSmartPointer<vtkUnsignedCharArray> ResultFrame;
  vtkTypeBool SwapBuffers;
  double DesiredUpdateRate;
  int AbortRender;
  int InAbortCheck;
  int InRender;
  int NeverRendered;
  int NumberOfLayers;
  int CurrentCursor;
  float AnaglyphColorSaturation;
  int AnaglyphColorMask[2];
  int MultiSamples;
  vtkTypeBool StencilCapable;
  int CapturingGL2PSSpecialProps;
  int DeviceIndex;

  bool UseSRGBColorSpace;
  char* CursorFileName;

  /**
   * The universal time since the last abort check occurred.
   */
  double AbortCheckTime;

  vtkRenderWindow* SharedRenderWindow;

  // -Z axis of the Physical to World matrix
  double PhysicalViewDirection[3] = { 0.0, 0.0, -1.0 };
  // Y axis of the Physical to World matrix
  double PhysicalViewUp[3] = { 0.0, 1.0, 0.0 };
  // Inverse of the translation component of the Physical to World matrix, in mm
  double PhysicalTranslation[3] = { 0.0, 0.0, 0.0 };
  // Scale of the Physical to World matrix
  double PhysicalScale = 1.0;

  bool EnableTranslucentSurface = false;

  bool Initialized = false;

private:
  vtkRenderWindow(const vtkRenderWindow&) = delete;
  void operator=(const vtkRenderWindow&) = delete;

  vtkNew<vtkStereoCompositor> StereoCompositor;
};

VTK_ABI_NAMESPACE_END
#endif
