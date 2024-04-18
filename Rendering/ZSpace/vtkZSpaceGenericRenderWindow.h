// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkZSpaceGenericRenderWindow
 * @brief Generic render window for stereo rendering on ZSpace Inspire
 *
 * `vtkZSpaceGenericRenderWindow` is a subclass of `vtkGenericOpenGLRenderWindow` designed to
 * handle stereo rendering on zSpace Inspire. This class should not be used with pre-Inspire
 * hardware relying on quad-buffering. In such cases, please use a `vtkRenderWindow` instead.
 *
 * `StereoRender` should be enabled on this window in order to make it to work as expected.
 *
 * This class interacts with the zSpace Core Compatibility API under the hood, that takes care
 * of left and right images composition and final rendering into the mono backbuffer.
 * In more details, this class overrides several methods of `vtkRenderWindow` in order to:
 * - Use the `RenderFrameBuffer` and `DisplayFrameBuffer` to store the left and right
 * eye textures respectively
 * - Let the zSpace API to do the composition of the stereo image (instead of the
 * internal `StereoCompositor`)
 * - Avoid blitting to the backbuffer (the zSpace API takes care of it)
 *
 * This class have similar behavior than `vtkZSpaceWin32RenderWindow`, but is intended
 * to be used with one's own OpenGL context, like `vtkGenericOpenGLRenderWindow`.
 *
 * Please note that this window should be shown fullscreen in order to have a working
 * stereo effect. ZSpace Inspire stereo is done directly by the screen (stereo display)
 * and the pixels of the front buffer should perfectly fit the pixel grid of the screen
 * (i.e. resolution of the front buffer and the display should be equal).
 *
 * @sa vtkZSpaceWin32RenderWindow vtkRenderWindow vtkGenericOpenGLRenderWindow
 */

#ifndef vtkZSpaceGenericRenderWindow_h
#define vtkZSpaceGenericRenderWindow_h

#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkRenderingZSpaceModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGZSPACE_EXPORT vtkZSpaceGenericRenderWindow : public vtkGenericOpenGLRenderWindow
{
public:
  static vtkZSpaceGenericRenderWindow* New();
  vtkTypeMacro(vtkZSpaceGenericRenderWindow, vtkGenericOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Begin the rendering process.
   * Overriden to notify the zSpace SDK for the beginning of a frame
   * as early as possible. This is useful to improve tracking.
   */
  void Start() override;
  ///@}

  /**
   * Initialize VTK for rendering in a new OpenGL context.
   * Overriden to let the zSpace API create its internal OpenGL resources
   * as well.
   */
  void OpenGLInitContext() override;

  /**
   * When this function is called, rendering should have been done in
   * the left eye framebuffer. Simply bind the right eye framebuffer
   * as draw framebuffer to render right eye in it.
   */
  void StereoMidpoint() override;

  /**
   * When this function is called, rendering should have been done in
   * the right eye framebuffer. Simply bind the left eye framebuffer
   * as draw framebuffer to render left eye in it in the next render pass.
   */
  void StereoRenderComplete() override;

  /**
   * When this function is called, both left and right eyes should have been
   * rendered in left and right eye framebuffers. Submit the left and right
   * eye textures to the zSpace API to let it compose the final woven image.
   * The zSpace API is also responsible to blit the final image into the
   * backbuffer, so here we only fire a WindowFrameEvent to indicate that
   * the frontbuffer and the backbuffer can be swapped.
   */
  void Frame() override;

protected:
  vtkZSpaceGenericRenderWindow();
  ~vtkZSpaceGenericRenderWindow() override;

private:
  vtkZSpaceGenericRenderWindow(const vtkZSpaceGenericRenderWindow&) = delete;
  void operator=(const vtkZSpaceGenericRenderWindow&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
