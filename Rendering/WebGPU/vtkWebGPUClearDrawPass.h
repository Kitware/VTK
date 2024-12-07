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
   * Get/set the DoClear attribute. If true this clear pass will clear the color and depth buffer
   * before rendering. Il false, this pass will render on top of the existing color and depth buffer
   */
  vtkGetMacro(DoClear, bool);
  vtkSetMacro(DoClear, bool);
  ///@}
  wgpu::RenderPassEncoder Begin(const vtkRenderState* state) override;

protected:
  vtkWebGPUClearDrawPass();
  ~vtkWebGPUClearDrawPass() override;

private:
  bool DoClear = true;

  vtkWebGPUClearDrawPass(const vtkWebGPUClearDrawPass&) = delete;
  void operator=(const vtkWebGPUClearDrawPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
