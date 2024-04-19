// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSDL2WebGPURenderWindow
 * @brief   OpenGL rendering window
 *
 * vtkSDL2WebGPURenderWindow is a concrete implementation of the abstract
 * class vtkRenderWindow. vtkSDL2OpenGL2Renderer interfaces to the standard
 * OpenGL graphics library using SDL2
 */

#ifndef vtkSDL2WebGPURenderWindow_h
#define vtkSDL2WebGPURenderWindow_h

#if !defined(__EMSCRIPTEN__)
#error "vtkSDL2WebGPURenderWindow cannot be built without emscripten!"
#endif

#include "vtkWebGPURenderWindow.h"

#include "vtkRenderingWebGPUModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_EXPORT vtkSDL2WebGPURenderWindow : public vtkWebGPURenderWindow
{
public:
  static vtkSDL2WebGPURenderWindow* New();
  vtkTypeMacro(vtkSDL2WebGPURenderWindow, vtkWebGPURenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  bool Initialize() override;

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

  /**
   * Show or not Show the window
   */
  void SetShowWindow(bool val) override;

  ///@{
  /**
   * Set the size of the window in pixels.
   */
  void SetSize(int, int) override;
  void SetSize(int a[2]) override { this->SetSize(a[0], a[1]); }
  ///@}

  /**
   * Get the current size of the window in pixels.
   */
  int* GetSize() VTK_SIZEHINT(2) override;

  ///@{
  /**
   * Set the position of the window.
   */
  void SetPosition(int, int) override;
  void SetPosition(int a[2]) override { this->SetPosition(a[0], a[1]); }
  ///@}

  /**
   * Get the current size of the screen in pixels.
   */
  int* GetScreenSize() VTK_SIZEHINT(2) override;

  /**
   * Get the position in screen coordinates of the window.
   */
  int* GetPosition() VTK_SIZEHINT(2) override;

  /**
   * Set the name of the window. This appears at the top of the window
   * normally.
   */
  void SetWindowName(const char*) override;

  void* GetGenericWindowId() override { return (void*)this->WindowId; }
  void* GetGenericDrawable() override { return (void*)this->WindowId; }

  void MakeCurrent() override {}

  void ReleaseCurrent() override {}

  /**
   * Tells if this window is the current OpenGL context for the calling thread.
   */
  bool IsCurrent() override { return false; }

  /**
   * Clean up device contexts, rendering contexts, etc.
   */
  void Clean();

  /**
   * A termination method performed at the end of the rendering process
   * to do things like swapping buffers (if necessary) or similar actions.
   */
  void Frame() override;

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

protected:
  vtkSDL2WebGPURenderWindow();
  ~vtkSDL2WebGPURenderWindow() override;

  void* WindowId = nullptr;

  std::string MakeDefaultWindowNameWithBackend() override;
  void CleanUpRenderers();
  void CreateAWindow() override;
  void DestroyWindow() override;

private:
  vtkSDL2WebGPURenderWindow(const vtkSDL2WebGPURenderWindow&) = delete;
  void operator=(const vtkSDL2WebGPURenderWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
