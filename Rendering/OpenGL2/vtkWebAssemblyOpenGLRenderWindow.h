// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebAssemblyOpenGL2RenderWindow
 * @brief   OpenGL rendering window
 *
 * vtkWebAssemblyOpenGL2RenderWindow is a concrete implementation of the abstract
 * class vtkRenderWindow.
 */

#ifndef vtkWebAssemblyOpenGLRenderWindow_h
#define vtkWebAssemblyOpenGLRenderWindow_h

#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO
#include <stack>                       // for ivar

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkWebAssemblyOpenGLRenderWindow
  : public vtkOpenGLRenderWindow
{
public:
  static vtkWebAssemblyOpenGLRenderWindow* New();
  vtkTypeMacro(vtkWebAssemblyOpenGLRenderWindow, vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  void Initialize() override;

  /**
   * Finalize the rendering window.  This will shutdown all system-specific
   * resources.  After having called this, it should be possible to destroy
   * a window that was used for a SetWindowId() call without any ill effects.
   */
  void Finalize() override;

  /**
   * Change the window to fill the entire screen.
   */
  void SetFullScreen(vtkTypeBool) override;

  ///@{
  /**
   * Set the size of the window in pixels.
   */
  void SetSize(int, int) override;
  void SetSize(int a[2]) override { vtkOpenGLRenderWindow::SetSize(a); }
  ///@}

  ///@{
  /**
   * Set the position of the window.
   */
  void SetPosition(int x, int y) override { vtkOpenGLRenderWindow::SetPosition(x, y); }
  void SetPosition(int a[2]) override { vtkOpenGLRenderWindow::SetPosition(a); }
  ///@}

  /**
   * Get the current size of the screen in pixels.
   */
  int* GetScreenSize() VTK_SIZEHINT(2) override;

  /**
   * Get the position in screen coordinates of the window.
   */
  int* GetPosition() VTK_SIZEHINT(2) override;

  void* GetGenericDisplayId() override { return (void*)this->ContextId; }
  void* GetGenericWindowId() override { return (void*)this->ContextId; }
  void* GetGenericDrawable() override { return (void*)this->ContextId; }

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
   * Clean up device contexts, rendering contexts, etc.
   */
  void Clean();

  /**
   * A termination method performed at the end of the rendering process
   * to do things like swapping buffers (if necessary) or similar actions.
   */
  void Frame() override;

  ///@{
  /**
   * Ability to push and pop this window's context
   * as the current context. The idea being to
   * if needed make this window's context current
   * and when done releasing resources restore
   * the prior context
   */
  void PushContext() override;
  void PopContext() override;
  ///@}

  /**
   * Set the number of vertical syncs required between frames.
   * A value of 0 means swap buffers as quickly as possible
   * regardless of the vertical refresh. A value of 1 means swap
   * buffers in sync with the vertical refresh to eliminate tearing.
   * A value of -1 means use a value of 1 unless we missed a frame
   * in which case swap immediately. Returns true if the call
   * succeeded.
   */
  bool SetSwapControl(int i) override;

  /**
   * Get the size of the color buffer.
   * Returns 0 if not able to determine otherwise sets R G B and A into buffer.
   */
  int GetColorBufferSizes(int* rgba) override;

  ///@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   */
  void HideCursor() override;
  void ShowCursor() override;
  ///@}

  /**
   * Specify the selector of the canvas element in the DOM.
   */
  vtkGetStringMacro(CanvasSelector);
  vtkSetStringMacro(CanvasSelector);

  /**
   * These enums have a one-one correspondence with the webgpu enums.
   * They are here so that wrapped languages can make use of them.
   */
  enum class PowerPreferenceType
  {
    Default,
    LowPower,
    HighPerformance
  };

  ///@{
  /**
   * Set/Get the power preference of the graphics adapter.
   * Available options are:
   * - `Default`
   * - `LowPower`
   * - `HighPerformance`.
   * The default value is `Default` (most probably LowPower for your browser).
   * NOTE: Make sure to call this before the first call to Render if you wish to change the
   * preference.
   * WARNING: Changing the power preference after the render window is initialized has
   * no effect.
   */
  vtkSetEnumMacro(PowerPreference, PowerPreferenceType);
  vtkGetEnumMacro(PowerPreference, PowerPreferenceType);
  ///@}

  ///@{
  /**
   * Set preference for a high-performance or low-power device.
   * The default preference is a default (most probably - low-power device).
   * NOTE: Make sure to call this before the first call to Render if you wish to change the
   * preference.
   * WARNING: Changing the power preference after the render window is initialized has
   * no effect.
   */
  void PreferHighPerformanceAdapter() { PowerPreference = PowerPreferenceType::HighPerformance; }
  void PreferLowPowerAdapter() { PowerPreference = PowerPreferenceType::LowPower; }
  ///@}

  /**
   * Make the setter for UseOffscreenBuffers no-op.
   * Offscreen buffers end up displaying a black screen which is not very useful.
   */
  void SetUseOffScreenBuffers(bool) override {}

  /**
   * Make the setter for ShowWindow no-op.
   * This property is meaningless in a web browser context.
   */
  void SetShowWindow(bool) override {}

protected:
  vtkWebAssemblyOpenGLRenderWindow();
  ~vtkWebAssemblyOpenGLRenderWindow() override;

  unsigned long ContextId;
  std::stack<unsigned long> ContextStack;
  char* CanvasSelector = nullptr;
  PowerPreferenceType PowerPreference = PowerPreferenceType::Default;

  void CleanUpRenderers();
  void CreateAWindow() override;
  void DestroyWindow() override;

private:
  vtkWebAssemblyOpenGLRenderWindow(const vtkWebAssemblyOpenGLRenderWindow&) = delete;
  void operator=(const vtkWebAssemblyOpenGLRenderWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
