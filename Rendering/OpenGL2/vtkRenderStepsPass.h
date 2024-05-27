// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRenderStepsPass
 * @brief   Execute render passes sequentially.
 *
 * vtkRenderStepsPass executes a standard list of render passes sequentially.
 * This class allows to define a sequence of render passes at run time.
 * You can set a step to NULL in order to skip that step. Likewise you
 * can replace any of the default steps with your own step. Typically in
 * such a case you would get the current step, replace it with your own
 * and likely have your step call the current step as a delegate. For example
 * to replace the translucent step with a depthpeeling step you would get the
 * current tranlucent step and set it as a delegate on the depthpeeling step.
 * Then set this classes translparent step to the depthpeelnig step.
 *
 * @sa
 * vtkRenderPass
 */

#ifndef vtkRenderStepsPass_h
#define vtkRenderStepsPass_h

#include "vtkRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkSequencePass;
class vtkCameraPass;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkRenderStepsPass : public vtkRenderPass
{
public:
  static vtkRenderStepsPass* New();
  vtkTypeMacro(vtkRenderStepsPass, vtkRenderPass);
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
   * Get the RenderPass used for the Camera Step
   */
  vtkGetObjectMacro(CameraPass, vtkCameraPass);
  void SetCameraPass(vtkCameraPass*);
  ///@}

  ///@{
  /**
   * Get the RenderPass used for the Lights Step
   */
  vtkGetObjectMacro(LightsPass, vtkRenderPass);
  void SetLightsPass(vtkRenderPass*);
  ///@}

  ///@{
  /**
   * Get the RenderPass used for the Opaque Step
   */
  vtkGetObjectMacro(OpaquePass, vtkRenderPass);
  void SetOpaquePass(vtkRenderPass*);
  ///@}

  ///@{
  /**
   * Get the RenderPass used for the translucent Step
   */
  vtkGetObjectMacro(TranslucentPass, vtkRenderPass);
  void SetTranslucentPass(vtkRenderPass*);
  ///@}

  ///@{
  /**
   * Get the RenderPass used for the Volume Step
   */
  vtkGetObjectMacro(VolumetricPass, vtkRenderPass);
  void SetVolumetricPass(vtkRenderPass*);
  ///@}

  ///@{
  /**
   * Get the RenderPass used for the Overlay Step
   */
  vtkGetObjectMacro(OverlayPass, vtkRenderPass);
  void SetOverlayPass(vtkRenderPass*);
  ///@}

  ///@{
  /**
   * Get the RenderPass used for the PostProcess Step
   */
  vtkGetObjectMacro(PostProcessPass, vtkRenderPass);
  void SetPostProcessPass(vtkRenderPass*);
  ///@}

protected:
  vtkRenderStepsPass();
  ~vtkRenderStepsPass() override;

  vtkCameraPass* CameraPass;
  vtkRenderPass* LightsPass;
  vtkRenderPass* OpaquePass;
  vtkRenderPass* TranslucentPass;
  vtkRenderPass* VolumetricPass;
  vtkRenderPass* OverlayPass;
  vtkRenderPass* PostProcessPass;
  vtkSequencePass* SequencePass;

private:
  vtkRenderStepsPass(const vtkRenderStepsPass&) = delete;
  void operator=(const vtkRenderStepsPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
