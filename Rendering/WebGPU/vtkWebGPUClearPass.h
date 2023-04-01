/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkWebGPUClearPass.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
