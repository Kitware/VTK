// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUClearPass_h
#define vtkWebGPUClearPass_h

#include "vtkWebGPURenderPass.h"

#include "vtkRenderingWebGPUModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUClearPass : public vtkWebGPURenderPass
{
public:
  static vtkWebGPUClearPass* New();
  vtkTypeMacro(vtkWebGPUClearPass, vtkWebGPURenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Render(const vtkRenderState* state) override;

  wgpu::RenderPassEncoder Begin(const vtkRenderState* state) override;

protected:
  vtkWebGPUClearPass();
  ~vtkWebGPUClearPass() override;

private:
  vtkWebGPUClearPass(const vtkWebGPUClearPass&) = delete;
  void operator=(const vtkWebGPUClearPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
