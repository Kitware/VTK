// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPURenderPass_h
#define vtkWebGPURenderPass_h

#include "vtkRenderPass.h"

#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtk_wgpu.h"                 // for webgpu

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPURenderPass : public vtkRenderPass
{
public:
  vtkTypeMacro(vtkWebGPURenderPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Render(const vtkRenderState* state) override;

  virtual wgpu::RenderPassEncoder Begin(const vtkRenderState* state) = 0;
  virtual void End(const vtkRenderState* state, wgpu::RenderPassEncoder&& pass);

protected:
  vtkWebGPURenderPass();
  ~vtkWebGPURenderPass() override;

private:
  vtkWebGPURenderPass(const vtkWebGPURenderPass&) = delete;
  void operator=(const vtkWebGPURenderPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
