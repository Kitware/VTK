// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGenericOpenGLRenderWindow
 * @brief   platform independent render window
 *
 *
 * vtkGenericOpenGLRenderWindow provides a skeleton for implementing a render window
 * using one's own OpenGL context and drawable.
 * To be effective, one must register an observer for WindowMakeCurrentEvent,
 * WindowIsCurrentEvent and WindowFrameEvent.  When this class sends a WindowIsCurrentEvent,
 * the call data is an bool* which one can use to return whether the context is current.
 */

#ifndef vtkGenericOpenGLRenderWindow_h
#define vtkGenericOpenGLRenderWindow_h

#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_EXPORT vtkGenericOpenGLRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkGenericOpenGLRenderWindow* New();
  vtkTypeMacro(vtkGenericOpenGLRenderWindow, vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkGenericOpenGLRenderWindow();
  ~vtkGenericOpenGLRenderWindow() override;

public:
  //! Cleans up graphics resources allocated in the context for this VTK scene.
  void Finalize() override;

  //! flush the pending drawing operations
  //! Class user may to watch for WindowFrameEvent and act on it
  void Frame() override;

  //! Makes the context current.  It is the class user's
  //! responsibility to watch for WindowMakeCurrentEvent and set it current.
  void MakeCurrent() override;

  //! Returns if the context is current.  It is the class user's
  //! responsibility to watch for WindowIsCurrentEvent and set the bool* flag
  //! passed through the call data parameter.
  bool IsCurrent() override;

  //! Returns if OpenGL is supported.  It is the class user's
  //! responsibility to watch for WindowSupportsOpenGLEvent and set the int* flag
  //! passed through the call data parameter.
  int SupportsOpenGL() override;

  //! Returns if the context is direct.  It is the class user's
  //! responsibility to watch for WindowIsDirectEvent and set the int* flag
  //! passed through the call data parameter.
  vtkTypeBool IsDirect() override;

  // {@
  //! set the drawing buffers to use
  void SetFrontLeftBuffer(unsigned int);
  void SetFrontRightBuffer(unsigned int);
  void SetBackLeftBuffer(unsigned int);
  void SetBackRightBuffer(unsigned int);
  // }@

  void SetOwnContext(vtkTypeBool);

  //! no-op (for API compat with OpenGL1).
  void PushState() {}
  //! no-op (for API compat with OpenGL1).
  void PopState() {}

  // {@
  //! does nothing
  void SetWindowId(void*) override;
  void* GetGenericWindowId() override;
  void SetDisplayId(void*) override;
  void SetParentId(void*) override;
  void* GetGenericDisplayId() override;
  void* GetGenericParentId() override;
  void* GetGenericContext() override;
  void* GetGenericDrawable() override;
  void SetWindowInfo(const char*) override;
  void SetParentInfo(const char*) override;
  int* GetScreenSize() VTK_SIZEHINT(2) override;
  void HideCursor() override;
  void ShowCursor() override;
  void SetFullScreen(vtkTypeBool) override;
  void WindowRemap() override;
  vtkTypeBool GetEventPending() override;
  void SetNextWindowId(void*) override;
  void SetNextWindowInfo(const char*) override;
  void CreateAWindow() override;
  void DestroyWindow() override;
  // }@

  ///@{
  /**
   * Allow to update state within observer callback without changing
   * data argument and MTime.
   */
  void SetIsDirect(vtkTypeBool newValue);
  void SetSupportsOpenGL(int newValue);
  void SetIsCurrent(bool newValue);
  ///@}

  /**
   * Override the Render method to do some state management.
   * This method saves the OpenGL state before asking its child renderers to
   * render their image. Once this is done, the OpenGL state is restored.
   * \sa vtkOpenGLRenderWindow::SaveGLState()
   * \sa vtkOpenGLRenderWindow::RestoreGLState()
   */
  void Render() override;

  /**
   * Overridden to pass explicitly specified MaximumHardwareLineWidth, if any.
   */
  float GetMaximumHardwareLineWidth() override;

  ///@{
  /**
   * Specify a non-zero line width to force the hardware line width determined
   * by the window.
   */
  vtkSetClampMacro(ForceMaximumHardwareLineWidth, float, 0, VTK_FLOAT_MAX);
  vtkGetMacro(ForceMaximumHardwareLineWidth, float);
  ///@}

  ///@{
  /**
   * Set this to true to indicate that the context is now ready. For backwards
   * compatibility reasons, it's set to true by default. If set to false, the
   * `Render` call will be skipped entirely.
   */
  vtkSetMacro(ReadyForRendering, bool);
  vtkGetMacro(ReadyForRendering, bool);

  /**
   * Set the size of the screen in pixels.
   * An HDTV for example would be 1920 x 1080 pixels.
   */
  vtkSetVector2Macro(ScreenSize, int);

  /**
   * Overridden to invoke vtkCommand::CursorChangedEvent
   */
  void SetCurrentCursor(int cShape) override;

  // since we are using an external context it must
  // specify if the window is mapped or not.
  vtkSetMacro(Mapped, vtkTypeBool);

  /**
   * Initialize OpenGL for this window.
   */
  void OpenGLInit() override;

protected:
  /**
   * Overridden to not attempt to read pixels if `this->ReadyForRendering` is
   * false. In that case, this method will simply return `VTK_ERROR`. Otherwise,
   * the superclass' implementation will be called.
   */
  int ReadPixels(
    const vtkRecti& rect, int front, int glFormat, int glType, void* data, int right) override;

  int SetPixelData(
    int x1, int y1, int x2, int y2, unsigned char* data, int front, int right) override;
  int SetPixelData(
    int x1, int y1, int x2, int y2, vtkUnsignedCharArray* data, int front, int right) override;
  int SetRGBACharPixelData(
    int x1, int y1, int x2, int y2, unsigned char* data, int front, int blend, int right) override;
  int SetRGBACharPixelData(int x, int y, int x2, int y2, vtkUnsignedCharArray* data, int front,
    int blend = 0, int right = 0) override;

  int DirectStatus;
  int SupportsOpenGLStatus;
  bool CurrentStatus;
  float ForceMaximumHardwareLineWidth;
  bool ReadyForRendering;

private:
  vtkGenericOpenGLRenderWindow(const vtkGenericOpenGLRenderWindow&) = delete;
  void operator=(const vtkGenericOpenGLRenderWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
