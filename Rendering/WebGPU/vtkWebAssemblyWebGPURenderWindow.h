// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebAssemblyWebGPURenderWindow
 * @brief   OpenGL rendering window
 *
 * vtkWebAssemblyWebGPURenderWindow is a concrete implementation of the abstract
 * class vtkRenderWindow. vtkSDL2OpenGL2Renderer interfaces to the standard
 * OpenGL graphics library using SDL2
 */

#ifndef vtkWebAssemblyWebGPURenderWindow_h
#define vtkWebAssemblyWebGPURenderWindow_h

#if !defined(__EMSCRIPTEN__)
#error "vtkWebAssemblyWebGPURenderWindow cannot be built without emscripten!"
#endif

#include "vtkWebGPURenderWindow.h"

#include "vtkRenderingWebGPUModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_EXPORT vtkWebAssemblyWebGPURenderWindow : public vtkWebGPURenderWindow
{
public:
  static vtkWebAssemblyWebGPURenderWindow* New();
  vtkTypeMacro(vtkWebAssemblyWebGPURenderWindow, vtkWebGPURenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  bool WindowSetup() override;

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
  void SetSize(int a[2]) override { this->SetSize(a[0], a[1]); }
  ///@}

  /**
   * Get the current size of the screen in pixels.
   */
  int* GetScreenSize() VTK_SIZEHINT(2) override;

  /**
   * Get the position in screen coordinates of the window.
   */
  int* GetPosition() VTK_SIZEHINT(2) override;

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

  /**
   * Specify the selector of the canvas element in the DOM.
   */
  vtkGetStringMacro(CanvasSelector);
  vtkSetStringMacro(CanvasSelector);

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
  vtkWebAssemblyWebGPURenderWindow();
  ~vtkWebAssemblyWebGPURenderWindow() override;

  void* WindowId = nullptr;
  char* CanvasSelector = nullptr;

  std::string MakeDefaultWindowNameWithBackend() override;
  void CleanUpRenderers();
  void CreateAWindow() override;
  void DestroyWindow() override;

private:
  vtkWebAssemblyWebGPURenderWindow(const vtkWebAssemblyWebGPURenderWindow&) = delete;
  void operator=(const vtkWebAssemblyWebGPURenderWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
