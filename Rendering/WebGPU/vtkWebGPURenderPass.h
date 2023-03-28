/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPURenderPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
