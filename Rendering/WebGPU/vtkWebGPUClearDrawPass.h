// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUClearDrawPass_h
#define vtkWebGPUClearDrawPass_h

#include "vtkWebGPURenderPass.h"

#include "vtkRenderingWebGPUModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUClearDrawPass : public vtkWebGPURenderPass
{
public:
  static vtkWebGPUClearDrawPass* New();
  vtkTypeMacro(vtkWebGPUClearDrawPass, vtkWebGPURenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Render(const vtkRenderState* state) override;

  ///@{
  /**
   * Get/set the ClearColor attribute. If true this clear pass will clear the color buffer
   * before rendering. If false, this pass will render on top of the existing color buffer
   */
  vtkGetMacro(ClearColor, bool);
  vtkSetMacro(ClearColor, bool);
  ///@}

  ///@{
  /**
   * Get/set the ClearDepth attribute. If true this clear pass will clear the depth buffer
   * before rendering. If false, this pass will render on top of the existing depth buffer
   */
  vtkGetMacro(ClearDepth, bool);
  vtkSetMacro(ClearDepth, bool);
  ///@}

  ///@{
  /**
   * Get/set the ClearStencil attribute. If true this clear pass will clear the stencil buffer
   * before rendering. If false, this pass will render on top of the existing stencil buffer
   */
  vtkGetMacro(ClearStencil, bool);
  vtkSetMacro(ClearStencil, bool);
  ///@}

  wgpu::RenderPassEncoder Begin(const vtkRenderState* state) override;

protected:
  vtkWebGPUClearDrawPass();
  ~vtkWebGPUClearDrawPass() override;

private:
  bool ClearColor = true;
  bool ClearDepth = true;
  bool ClearStencil = true;

  vtkWebGPUClearDrawPass(const vtkWebGPUClearDrawPass&) = delete;
  void operator=(const vtkWebGPUClearDrawPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
