// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCameraPass
 * @brief   Implement the camera render pass.
 *
 * Render the camera.
 *
 * It setups the projection and modelview matrices and can clear the background
 * It calls its delegate once.
 * After its delegate returns, it restore the modelview matrix stack.
 *
 * Its delegate is usually set to a vtkSequencePass with a vtkLightsPass and
 * a list of passes for the geometry.
 *
 * @sa
 * vtkRenderPass
 */

#ifndef vtkCameraPass_h
#define vtkCameraPass_h

#include "vtkRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkCameraPass : public vtkRenderPass
{
public:
  static vtkCameraPass* New();
  vtkTypeMacro(vtkCameraPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  ///@{
  /**
   * Delegate for rendering the geometry.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It is usually set to a vtkSequencePass with a vtkLightsPass and
   * a list of passes for the geometry.
   * Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(DelegatePass, vtkRenderPass);
  virtual void SetDelegatePass(vtkRenderPass* delegatePass);
  ///@}

  ///@{
  /**
   * Used to override the aspect ratio used when computing the projection
   * matrix. This is useful when rendering for tile-displays for example.
   */
  vtkSetMacro(AspectRatioOverride, double);
  vtkGetMacro(AspectRatioOverride, double);

protected:
  ///@}
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  vtkCameraPass();

  ///@{
  /**
   * Destructor.
   */
  ~vtkCameraPass() override;
  virtual void GetTiledSizeAndOrigin(
    const vtkRenderState* render_state, int* width, int* height, int* originX, int* originY);
  ///@}

  vtkRenderPass* DelegatePass;

  double AspectRatioOverride;

private:
  vtkCameraPass(const vtkCameraPass&) = delete;
  void operator=(const vtkCameraPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
