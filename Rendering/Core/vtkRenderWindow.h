/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWindow.h"

class vtkFloatArray;
class vtkPainterDeviceAdapter;
class vtkProp;
class vtkCollection;
class vtkRenderWindowInteractor;
class vtkRenderer;
class vtkRendererCollection;
class vtkUnsignedCharArray;

// lets define the different types of stereo
#define VTK_STEREO_CRYSTAL_EYES 1
#define VTK_STEREO_RED_BLUE     2
#define VTK_STEREO_INTERLACED   3
#define VTK_STEREO_LEFT         4
#define VTK_STEREO_RIGHT        5
#define VTK_STEREO_DRESDEN      6
#define VTK_STEREO_ANAGLYPH     7
#define VTK_STEREO_CHECKERBOARD 8
#define VTK_STEREO_SPLITVIEWPORT_HORIZONTAL 9
#define VTK_STEREO_FAKE 10

#define VTK_CURSOR_DEFAULT   0
#define VTK_CURSOR_ARROW     1
#define VTK_CURSOR_SIZENE    2
#define VTK_CURSOR_SIZENW    3
#define VTK_CURSOR_SIZESW    4
#define VTK_CURSOR_SIZESE    5
#define VTK_CURSOR_SIZENS    6
#define VTK_CURSOR_SIZEWE    7
#define VTK_CURSOR_SIZEALL   8
#define VTK_CURSOR_HAND      9
#define VTK_CURSOR_CROSSHAIR 10

class VTKRENDERINGCORE_EXPORT vtkRenderWindow : public vtkWindow
{
public:
  vtkTypeMacro(vtkRenderWindow,vtkWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Construct an instance of  vtkRenderWindow with its screen size
   * set to 300x300, borders turned on, positioned at (0,0), double
   * buffering turned on.
   */
  static vtkRenderWindow *New();

  /**
   * Add a renderer to the list of renderers.
   */
  virtual void AddRenderer(vtkRenderer *);

  /**
   * Remove a renderer from the list of renderers.
   */
  void RemoveRenderer(vtkRenderer *);

  /**
   * Query if a renderer is in the list of renderers.
   */
  int HasRenderer(vtkRenderer *);

  /**
   * What rendering library has the user requested
   */
  static const char *GetRenderLibrary();

  /**
   * What rendering backend has the user requested
   */
  virtual const char *GetRenderingBackend();

  /**
   * Return the collection of renderers in the render window.
   */
  vtkRendererCollection *GetRenderers() {return this->Renderers;};

  /**
   * The GL2PS exporter must handle certain props in a special way (e.g. text).
   * This method performs a render and captures all "GL2PS-special" props in
   * the specified collection. The collection will contain a
   * vtkPropCollection for each vtkRenderer in this->GetRenderers(), each
   * containing the special props rendered by the corresponding renderer.
   */
  void CaptureGL2PSSpecialProps(vtkCollection *specialProps);

  //@{
  /**
   * Returns true if the render process is capturing text actors.
   */
  vtkGetMacro(CapturingGL2PSSpecialProps, int);
  //@}

  /**
   * Ask each renderer owned by this RenderWindow to render its image and
   * synchronize this process.
   */
  virtual void Render();

  /**
   * Initialize the rendering process.
   */
  virtual void Start() = 0;

  /**
   * Finalize the rendering process.
   */
  virtual void Finalize() = 0;

  /**
   * A termination method performed at the end of the rendering process
   * to do things like swapping buffers (if necessary) or similar actions.
   */
  virtual void Frame() = 0;

  /**
   * Block the thread until the actual rendering is finished().
   * Useful for measurement only.
   */
  virtual void WaitForCompletion()=0;

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
  virtual vtkRenderWindowInteractor *MakeRenderWindowInteractor();

  //@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   * Set cursor position in window (note that (0,0) is the lower left
   * corner).
   */
  virtual void HideCursor() = 0;
  virtual void ShowCursor() = 0;
  virtual void SetCursorPosition(int , int ) {}
  //@}

  //@{
  /**
   * Change the shape of the cursor.
   */
  vtkSetMacro(CurrentCursor,int);
  vtkGetMacro(CurrentCursor,int);
  //@}

  //@{
  /**
   * Turn on/off rendering full screen window size.
   */
  virtual void SetFullScreen(int) = 0;
  vtkGetMacro(FullScreen,int);
  vtkBooleanMacro(FullScreen,int);
  //@}

  //@{
  /**
   * Turn on/off window manager borders. Typically, you shouldn't turn the
   * borders off, because that bypasses the window manager and can cause
   * undesirable behavior.
   */
  vtkSetMacro(Borders,int);
  vtkGetMacro(Borders,int);
  vtkBooleanMacro(Borders,int);
  //@}

  //@{
  /**
   * Prescribe that the window be created in a stereo-capable mode. This
   * method must be called before the window is realized. Default is off.
   */
  vtkGetMacro(StereoCapableWindow,int);
  vtkBooleanMacro(StereoCapableWindow,int);
  virtual void SetStereoCapableWindow(int capable);
  //@}

  //@{
  /**
   * Turn on/off stereo rendering.
   */
  vtkGetMacro(StereoRender,int);
  void SetStereoRender(int stereo);
  vtkBooleanMacro(StereoRender,int);
  //@}

  //@{
  /**
   * Turn on/off the use of alpha bitplanes.
   */
  vtkSetMacro(AlphaBitPlanes, int);
  vtkGetMacro(AlphaBitPlanes, int);
  vtkBooleanMacro(AlphaBitPlanes, int);
  //@}

  //@{
  /**
   * Turn on/off point smoothing. Default is off.
   * This must be applied before the first Render.
   */
  vtkSetMacro(PointSmoothing,int);
  vtkGetMacro(PointSmoothing,int);
  vtkBooleanMacro(PointSmoothing,int);
  //@}

  //@{
  /**
   * Turn on/off line smoothing. Default is off.
   * This must be applied before the first Render.
   */
  vtkSetMacro(LineSmoothing,int);
  vtkGetMacro(LineSmoothing,int);
  vtkBooleanMacro(LineSmoothing,int);
  //@}

  //@{
  /**
   * Turn on/off polygon smoothing. Default is off.
   * This must be applied before the first Render.
   */
  vtkSetMacro(PolygonSmoothing,int);
  vtkGetMacro(PolygonSmoothing,int);
  vtkBooleanMacro(PolygonSmoothing,int);
  //@}

  //@{
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
   * actually rendering in stereo mode.
   */
  vtkGetMacro(StereoType,int);
  vtkSetMacro(StereoType,int);
  void SetStereoTypeToCrystalEyes()
    {this->SetStereoType(VTK_STEREO_CRYSTAL_EYES);}
  void SetStereoTypeToRedBlue()
    {this->SetStereoType(VTK_STEREO_RED_BLUE);}
  void SetStereoTypeToInterlaced()
    {this->SetStereoType(VTK_STEREO_INTERLACED);}
  void SetStereoTypeToLeft()
    {this->SetStereoType(VTK_STEREO_LEFT);}
  void SetStereoTypeToRight()
    {this->SetStereoType(VTK_STEREO_RIGHT);}
  void SetStereoTypeToDresden()
    {this->SetStereoType(VTK_STEREO_DRESDEN);}
  void SetStereoTypeToAnaglyph()
    {this->SetStereoType(VTK_STEREO_ANAGLYPH);}
  void SetStereoTypeToCheckerboard()
    {this->SetStereoType(VTK_STEREO_CHECKERBOARD);}
  void SetStereoTypeToSplitViewportHorizontal()
    {this->SetStereoType(VTK_STEREO_SPLITVIEWPORT_HORIZONTAL);}
  void SetStereoTypeToFake()
    {this->SetStereoType(VTK_STEREO_FAKE);}
  //@}

  const char *GetStereoTypeAsString();

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

  //@{
  /**
   * Set/get the anaglyph color saturation factor.  This number ranges from
   * 0.0 to 1.0:  0.0 means that no color from the original object is
   * maintained, 1.0 means all of the color is maintained.  The default
   * value is 0.65.  Too much saturation can produce uncomfortable 3D
   * viewing because anaglyphs also use color to encode 3D.
   */
  vtkSetClampMacro(AnaglyphColorSaturation,float, 0.0f, 1.0f);
  vtkGetMacro(AnaglyphColorSaturation,float);
  //@}

  //@{
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
  vtkSetVector2Macro(AnaglyphColorMask,int);
  vtkGetVectorMacro(AnaglyphColorMask,int,2);
  //@}

  /**
   * Remap the rendering window. This probably only works on UNIX right now.
   * It is useful for changing properties that can't normally be changed
   * once the window is up.
   */
  virtual void WindowRemap() = 0;

  //@{
  /**
   * Turn on/off buffer swapping between images.
   */
  vtkSetMacro(SwapBuffers,int);
  vtkGetMacro(SwapBuffers,int);
  vtkBooleanMacro(SwapBuffers,int);
  //@}

  //@{
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
  virtual int SetPixelData(int x, int y, int x2, int y2, unsigned char *data,
                           int front) = 0;
  virtual int SetPixelData(int x, int y, int x2, int y2,
                           vtkUnsignedCharArray *data, int front) = 0;
  //@}

  //@{
  /**
   * Same as Get/SetPixelData except that the image also contains an alpha
   * component. The image is transmitted as RGBARGBARGBA... each of which is a
   * float value. The "blend" parameter controls whether the SetRGBAPixelData
   * method blends the data with the previous contents of the frame buffer
   * or completely replaces the frame buffer data.
   */
  virtual float *GetRGBAPixelData(int x, int y, int x2, int y2, int front) = 0;
  virtual int GetRGBAPixelData(int x, int y, int x2, int y2, int front,
                               vtkFloatArray *data) = 0;
  virtual int SetRGBAPixelData(int x, int y, int x2, int y2, float *,
                               int front, int blend=0) = 0;
  virtual int SetRGBAPixelData(int, int, int, int, vtkFloatArray*,
                               int, int blend=0) = 0;
  virtual void ReleaseRGBAPixelData(float *data)=0;
  virtual unsigned char *GetRGBACharPixelData(int x, int y, int x2, int y2,
                                              int front) = 0;
  virtual int GetRGBACharPixelData(int x, int y, int x2, int y2, int front,
                                   vtkUnsignedCharArray *data) = 0;
  virtual int SetRGBACharPixelData(int x,int y, int x2, int y2,
                                   unsigned char *data, int front,
                                   int blend=0) = 0;
  virtual int SetRGBACharPixelData(int x, int y, int x2, int y2,
                                   vtkUnsignedCharArray *data, int front,
                                   int blend=0) = 0;
  //@}

  //@{
  /**
   * Set/Get the zbuffer data from the frame buffer.
   * (x,y) is any corner of the rectangle. (x2,y2) is its opposite corner on
   * the diagonal.
   */
  virtual float *GetZbufferData(int x, int y, int x2, int y2) = 0;
  virtual int GetZbufferData(int x, int y, int x2, int y2, float *z) = 0;
  virtual int GetZbufferData(int x, int y, int x2, int y2,
                             vtkFloatArray *z) = 0;
  virtual int SetZbufferData(int x, int y, int x2, int y2, float *z) = 0;
  virtual int SetZbufferData(int x, int y, int x2, int y2,
                             vtkFloatArray *z) = 0;
  float GetZbufferDataAtPoint(int x, int y)
  {
    float value;
    this->GetZbufferData(x, y, x, y, &value);
    return value;
  }
  //@}

  //@{
  /**
   * Set the number of frames for doing antialiasing. The default is
   * zero. Typically five or six will yield reasonable results without
   * taking too long.
   */
  vtkGetMacro(AAFrames,int);
  vtkSetMacro(AAFrames,int);
  //@}

  //@{
  /**
   * Set the number of frames for doing focal depth. The default is zero.
   * Depending on how your scene is organized you can get away with as
   * few as four frames for focal depth or you might need thirty.
   * One thing to note is that if you are using focal depth frames,
   * then you will not need many (if any) frames for antialiasing.
   */
  vtkGetMacro(FDFrames,int);
  virtual void SetFDFrames (int fdFrames);
  //@}

  //@{
  /**
   * Turn on/off using constant offsets for focal depth rendering.
   * The default is off. When constants offsets are used, re-rendering
   * the same scene using the same camera yields the same image; otherwise
   * offsets are random numbers at each rendering that yields
   * slightly different images.
   */
  vtkGetMacro(UseConstantFDOffsets,int);
  vtkSetMacro(UseConstantFDOffsets,int);
  //@}

  //@{
  /**
   * Set the number of sub frames for doing motion blur. The default is zero.
   * Once this is set greater than one, you will no longer see a new frame
   * for every Render().  If you set this to five, you will need to do
   * five Render() invocations before seeing the result. This isn't
   * very impressive unless something is changing between the Renders.
   * Changing this value may reset the current subframe count.
   */
  vtkGetMacro(SubFrames,int);
  virtual void SetSubFrames(int subFrames);
  //@}

  //@{
  /**
   * This flag is set if the window hasn't rendered since it was created
   */
  vtkGetMacro(NeverRendered,int);
  //@}

  //@{
  /**
   * This is a flag that can be set to interrupt a rendering that is in
   * progress.
   */
  vtkGetMacro(AbortRender,int);
  vtkSetMacro(AbortRender,int);
  vtkGetMacro(InAbortCheck,int);
  vtkSetMacro(InAbortCheck,int);
  virtual int CheckAbortStatus();
  //@}

  vtkGetMacro(IsPicking,int);
  vtkSetMacro(IsPicking,int);
  vtkBooleanMacro(IsPicking,int);

  /**
   * Check to see if a mouse button has been pressed.  All other events
   * are ignored by this method.  Ideally, you want to abort the render
   * on any event which causes the DesiredUpdateRate to switch from
   * a high-quality rate to a more interactive rate.
   */
  virtual int GetEventPending() = 0;

  /**
   * Are we rendering at the moment
   */
  virtual int  CheckInRenderStatus() { return this->InRender; }

  /**
   * Clear status (after an exception was thrown for example)
   */
  virtual void ClearInRenderStatus() { this->InRender = 0; }

  //@{
  /**
   * Set/Get the desired update rate. This is used with
   * the vtkLODActor class. When using level of detail actors you
   * need to specify what update rate you require. The LODActors then
   * will pick the correct resolution to meet your desired update rate
   * in frames per second. A value of zero indicates that they can use
   * all the time they want to.
   */
  virtual void SetDesiredUpdateRate(double);
  vtkGetMacro(DesiredUpdateRate,double);
  //@}

  //@{
  /**
   * Get the number of layers for renderers.  Each renderer should have
   * its layer set individually.  Some algorithms iterate through all layers,
   * so it is not wise to set the number of layers to be exorbitantly large
   * (say bigger than 100).
   */
  vtkGetMacro(NumberOfLayers, int);
  vtkSetClampMacro(NumberOfLayers, int, 1, VTK_INT_MAX);
  //@}

  //@{
  /**
   * Get the interactor associated with this render window
   */
  vtkGetObjectMacro(Interactor,vtkRenderWindowInteractor);
  //@}

  /**
   * Set the interactor to the render window
   */
  void SetInteractor(vtkRenderWindowInteractor *);

  /**
   * This Method detects loops of RenderWindow<->Interactor,
   * so objects are freed properly.
   */
  virtual void UnRegister(vtkObjectBase *o);

  //@{
  /**
   * Dummy stubs for vtkWindow API.
   */
  virtual void SetDisplayId(void *) = 0;
  virtual void SetWindowId(void *)  = 0;
  virtual void SetNextWindowId(void *) = 0;
  virtual void SetParentId(void *)  = 0;
  virtual void *GetGenericDisplayId() = 0;
  virtual void *GetGenericWindowId() = 0;
  virtual void *GetGenericParentId() = 0;
  virtual void *GetGenericContext() = 0;
  virtual void *GetGenericDrawable() = 0;
  virtual void SetWindowInfo(char *) = 0;
  virtual void SetNextWindowInfo(char *) = 0;
  virtual void SetParentInfo(char *) = 0;
  //@}

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  virtual bool InitializeFromCurrentContext() { return false; };

  /**
   * Attempt to make this window the current graphics context for the calling
   * thread.
   */
  virtual void MakeCurrent() = 0;

  /**
   * Tells if this window is the current graphics context for the calling
   * thread.
   */
  virtual bool IsCurrent()=0;

  /**
   * Test if the window has a valid drawable. This is
   * currently only an issue on Mac OS X Cocoa where rendering
   * to an invalid drawable results in all OpenGL calls to fail
   * with "invalid framebuffer operation".
   */
  virtual bool IsDrawable(){ return true; }

  /**
   * If called, allow MakeCurrent() to skip cache-check when called.
   * MakeCurrent() reverts to original behavior of cache-checking
   * on the next render.
   */
  virtual void SetForceMakeCurrent() {}

  /**
   * Get report of capabilities for the render window
   */
  virtual const char *ReportCapabilities() { return "Not Implemented";};

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  virtual int SupportsOpenGL() { return 0;};

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  virtual int IsDirect() { return 0;};

  /**
   * This method should be defined by the subclass. How many bits of
   * precision are there in the zbuffer?
   */
  virtual int GetDepthBufferSize() = 0;

  /**
   * Get the size of the color buffer.
   * Returns 0 if not able to determine otherwise sets R G B and A into buffer.
   */
  virtual int GetColorBufferSizes(int *rgba) = 0;

  //@{
  /**
   * Get the vtkPainterDeviceAdapter which can be used to paint on
   * this render window.  Note the old OpenGL backend requires this
   * method.
   */
  vtkGetObjectMacro(PainterDeviceAdapter, vtkPainterDeviceAdapter);
  //@}

  //@{
  /**
   * Set / Get the number of multisamples to use for hardware antialiasing.
   */
  vtkSetMacro(MultiSamples,int);
  vtkGetMacro(MultiSamples,int);
  //@}

  //@{
  /**
   * Set / Get the availability of the stencil buffer.
   */
  vtkSetMacro(StencilCapable, int);
  vtkGetMacro(StencilCapable, int);
  vtkBooleanMacro(StencilCapable, int);
  //@}

  //@{
  /**
   * If there are several graphics card installed on a system,
   * this index can be used to specify which card you want to render to.
   * the default is 0. This may not work on all derived render window and
   * it may need to be set before the first render.
   */
  vtkSetMacro(DeviceIndex,int);
  vtkGetMacro(DeviceIndex,int);
  //@}
  /**
   * Returns the number of devices (graphics cards) on a system.
   * This may not work on all derived render windows.
   */
  virtual int GetNumberOfDevices()
  {
    return 0;
  }

  /**
   * Create and bind offscreen rendering buffers without destroying the current
   * OpenGL context. This allows to temporary switch to offscreen rendering
   * (ie. to make a screenshot even if the window is hidden).
   * Return if the creation was successful (1) or not (0).
   * Note: This function requires that the device supports OpenGL framebuffer extension.
   * The function has no effect if OffScreenRendering is ON.
   */
  virtual int SetUseOffScreenBuffers(bool) { return 0; }
  virtual bool GetUseOffScreenBuffers() { return false; }

protected:
  vtkRenderWindow();
  ~vtkRenderWindow();

  virtual void DoStereoRender();
  virtual void DoFDRender();
  virtual void DoAARender();

  vtkPainterDeviceAdapter* PainterDeviceAdapter;
  vtkRendererCollection *Renderers;
  int Borders;
  int FullScreen;
  int OldScreen[5];
  int PointSmoothing;
  int LineSmoothing;
  int PolygonSmoothing;
  int StereoRender;
  int StereoType;
  int StereoStatus; // used for keeping track of what's going on
  int StereoCapableWindow;
  int AlphaBitPlanes;
  vtkRenderWindowInteractor *Interactor;
  unsigned char* StereoBuffer; // used for red blue stereo
  float *AccumulationBuffer;   // used for many techniques
  unsigned int AccumulationBufferSize;
  int AAFrames;
  int FDFrames;
  int UseConstantFDOffsets; // to use the same offsets at each rendering
  double *ConstantFDOffsets[2];
  int SubFrames;               // number of sub frames
  int CurrentSubFrame;         // what one are we on
  unsigned char *ResultFrame;  // used for any non immediate rendering
  int   SwapBuffers;
  double DesiredUpdateRate;
  int   AbortRender;
  int   InAbortCheck;
  int   InRender;
  int   NeverRendered;
  int   NumberOfLayers;
  int CurrentCursor;
  int IsPicking;
  float AnaglyphColorSaturation;
  int AnaglyphColorMask[2];
  int MultiSamples;
  int StencilCapable;
  int CapturingGL2PSSpecialProps;
  int DeviceIndex;

  /**
   * The universal time since the last abort check occurred.
   */
  double AbortCheckTime;

private:
  vtkRenderWindow(const vtkRenderWindow&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRenderWindow&) VTK_DELETE_FUNCTION;
};

#endif
