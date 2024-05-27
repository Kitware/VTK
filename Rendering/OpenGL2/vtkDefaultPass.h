// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDefaultPass
 * @brief   Implement the basic render passes.
 *
 * vtkDefaultPass implements the basic standard render passes of VTK.
 * Subclasses can easily be implemented by reusing some parts of the basic
 * implementation.
 *
 * It implements classic Render operations as well as versions with property
 * key checking.
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farthest z-value and
 * background color/gradient/transparent color.
 *
 * @sa
 * vtkRenderPass
 */

#ifndef vtkDefaultPass_h
#define vtkDefaultPass_h

#include "vtkRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;
class vtkDefaultPassLayerList; // Pimpl

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkDefaultPass : public vtkRenderPass
{
public:
  static vtkDefaultPass* New();
  vtkTypeMacro(vtkDefaultPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * Call RenderOpaqueGeometry(), RenderTranslucentPolygonalGeometry(),
   * RenderVolumetricGeometry(), RenderOverlay()
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState* s) override;

protected:
  /**
   * Default constructor.
   */
  vtkDefaultPass();

  /**
   * Destructor.
   */
  ~vtkDefaultPass() override;

  /**
   * Opaque pass without key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderOpaqueGeometry(const vtkRenderState* s);

  /**
   * Opaque pass with key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderFilteredOpaqueGeometry(const vtkRenderState* s);

  /**
   * Translucent pass without key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderTranslucentPolygonalGeometry(const vtkRenderState* s);

  /**
   * Translucent pass with key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderFilteredTranslucentPolygonalGeometry(const vtkRenderState* s);

  /**
   * Volume pass without key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderVolumetricGeometry(const vtkRenderState* s);

  /**
   * Translucent pass with key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderFilteredVolumetricGeometry(const vtkRenderState* s);

  /**
   * Overlay pass without key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderOverlay(const vtkRenderState* s);

  /**
   * Overlay pass with key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderFilteredOverlay(const vtkRenderState* s);

private:
  vtkDefaultPass(const vtkDefaultPass&) = delete;
  void operator=(const vtkDefaultPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
