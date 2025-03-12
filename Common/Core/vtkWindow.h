// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWindow
 * @brief   window superclass for vtkRenderWindow
 *
 * vtkWindow is an abstract object to specify the behavior of a
 * rendering window.  It contains vtkViewports.
 *
 * @sa
 * vtkRenderWindow vtkViewport
 */

#ifndef vtkWindow_h
#define vtkWindow_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkUnsignedCharArray;

class VTKCOMMONCORE_EXPORT VTK_MARSHALAUTO vtkWindow : public vtkObject
{
public:
  vtkTypeMacro(vtkWindow, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * These are window system independent methods that are used
   * to help interface vtkWindow to native windowing systems.
   */
  virtual void SetDisplayId(void*) {}
  virtual void SetWindowId(void*) {}
  virtual void SetParentId(void*) {}
  virtual void* GetGenericDisplayId() { return nullptr; }
  virtual void* GetGenericWindowId() { return nullptr; }
  virtual void* GetGenericParentId() { return nullptr; }
  virtual void* GetGenericContext() { return nullptr; }
  virtual void* GetGenericDrawable() { return nullptr; }
  virtual void SetWindowInfo(const char*) {}
  virtual void SetParentInfo(const char*) {}
  virtual bool EnsureDisplay() { return true; }

  ///@}

  ///@{
  /**
   * Get the position (x and y) of the rendering window in
   * screen coordinates (in pixels).
   */
  virtual int* GetPosition() VTK_SIZEHINT(2);

  /**
   * Set the position (x and y) of the rendering window in
   * screen coordinates (in pixels). This resizes the operating
   * system's view/window and redraws it.
   */
  virtual void SetPosition(int x, int y);
  virtual void SetPosition(int a[2]);
  ///@}

  ///@{
  /**
   * Get the size (width and height) of the rendering window in
   * screen coordinates (in pixels).
   */
  virtual int* GetSize() VTK_SIZEHINT(2);

  /**
   * Set the size (width and height) of the rendering window in
   * screen coordinates (in pixels). This resizes the operating
   * system's view/window and redraws it.
   *
   * If the size has changed, this method will fire
   * vtkCommand::WindowResizeEvent.
   */
  virtual void SetSize(int width, int height);
  virtual void SetSize(int a[2]);
  ///@}

  /**
   * GetSize() returns the size * this->TileScale, whereas this method returns
   * the size without multiplying with the tile scale. Measured in pixels.
   */
  int* GetActualSize() VTK_SIZEHINT(2);

  /**
   * Get the current size of the screen in pixels.
   */
  virtual int* GetScreenSize() VTK_SIZEHINT(2) { return nullptr; }

  ///@{
  /**
   * Keep track of whether the rendering window has been mapped to screen.
   */
  vtkGetMacro(Mapped, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Show or not Show the window
   */
  vtkGetMacro(ShowWindow, bool);
  vtkSetMacro(ShowWindow, bool);
  vtkBooleanMacro(ShowWindow, bool);
  ///@}

  ///@{
  /**
   * Render to an offscreen destination such as a framebuffer.
   * All four combinations of ShowWindow and UseOffScreenBuffers
   * should work for most rendering backends.
   */
  vtkSetMacro(UseOffScreenBuffers, bool);
  vtkGetMacro(UseOffScreenBuffers, bool);
  vtkBooleanMacro(UseOffScreenBuffers, bool);
  ///@}

  ///@{
  /**
   * Turn on/off erasing the screen between images. This allows multiple
   * exposure sequences if turned on. You will need to turn double
   * buffering off or make use of the SwapBuffers methods to prevent
   * you from swapping buffers between exposures.
   */
  vtkSetMacro(Erase, vtkTypeBool);
  vtkGetMacro(Erase, vtkTypeBool);
  vtkBooleanMacro(Erase, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Keep track of whether double buffering is on or off
   */
  vtkSetMacro(DoubleBuffer, vtkTypeBool);
  vtkGetMacro(DoubleBuffer, vtkTypeBool);
  vtkBooleanMacro(DoubleBuffer, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Get name of rendering window
   */
  vtkGetStringMacro(WindowName);
  vtkSetStringMacro(WindowName);
  ///@}

  /**
   * Set the icon used in title bar and task bar.
   * Currently implemented for OpenGL windows on Windows and Linux.
   */
  virtual void SetIcon(vtkImageData*) {}

  /**
   * Ask each viewport owned by this Window to render its image and
   * synchronize this process.
   */
  VTK_UNBLOCKTHREADS
  virtual void Render() {}

  /**
   * Release any graphics resources that are being consumed by this texture.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow*) {}

  ///@{
  /**
   * Get the pixel data of an image, transmitted as RGBRGBRGB. The
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
  virtual unsigned char* GetPixelData(
    int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, int /*front*/, int /*right*/ = 0)
  {
    return nullptr;
  }
  virtual int GetPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, int /*front*/,
    vtkUnsignedCharArray* /*data*/, int /*right*/ = 0)
  {
    return 0;
  }
  ///@}

  ///@{
  /**
   * Return a best estimate to the dots per inch of the display
   * device being rendered (or printed).
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  vtkGetMacro(DPI, int);
  vtkSetClampMacro(DPI, int, 1, VTK_INT_MAX);
  ///@}

  /**
   * Attempt to detect and set the DPI of the display device by querying the
   * system. Note that this is not supported on most backends, and this method
   * will return false if the DPI could not be detected. Use GetDPI() to
   * inspect the detected value.
   */
  virtual bool DetectDPI() { return false; }

  ///@{
  /**
   * Convenience to set SHowWindow and UseOffScreenBuffers in one call
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void SetOffScreenRendering(vtkTypeBool val)
  {
    this->SetShowWindow(val == 0);
    this->SetUseOffScreenBuffers(val != 0);
  }
  vtkBooleanMacro(OffScreenRendering, vtkTypeBool);
  ///@}

  /**
   * Deprecated, directly use GetShowWindow and GetOffScreenBuffers
   * instead.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  vtkTypeBool GetOffScreenRendering() { return this->GetShowWindow() ? 0 : 1; }

  /**
   * Make the window current. May be overridden in subclasses to do
   * for example a glXMakeCurrent or a wglMakeCurrent.
   */
  virtual void MakeCurrent() {}

  /**
   * Release the current context. May be overridden in subclasses to do
   * for example a glXMakeCurrent or a wglMakeCurrent.
   */
  virtual void ReleaseCurrent() {}

  ///@{
  /**
   * These methods are used by vtkWindowToImageFilter to tell a VTK window
   * to simulate a larger window by tiling. For 3D geometry these methods
   * have no impact. It is just in handling annotation that this information
   * must be available to the mappers and the coordinate calculations.
   */
  vtkSetVector2Macro(TileScale, int);
  vtkGetVector2Macro(TileScale, int);
  void SetTileScale(int s) { this->SetTileScale(s, s); }
  vtkSetVector4Macro(TileViewport, double);
  vtkGetVector4Macro(TileViewport, double);
  ///@}

protected:
  vtkWindow();
  ~vtkWindow() override;

  char* WindowName;
  int Size[2];
  int Position[2];
  vtkTypeBool Mapped;
  bool ShowWindow;
  bool UseOffScreenBuffers;
  vtkTypeBool Erase;
  vtkTypeBool DoubleBuffer;
  int DPI;

  double TileViewport[4];
  int TileSize[2];
  int TileScale[2];

private:
  vtkWindow(const vtkWindow&) = delete;
  void operator=(const vtkWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
