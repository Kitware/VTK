// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUClearRGBPass
 * @brief   WebGPU clear RGB pass.
 *
 * vtkWebGPUClearRGBPass is the WebGPU implementation of vtkClearRGBPass.
 *
 * @sa
 * vtkClearRGBPass
 */

#ifndef vtkWebGPUClearRGBPass_h
#define vtkWebGPUClearRGBPass_h

#include "vtkClearRGBPass.h"
#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkWrappingHints.h"         // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOverrideAttribute;

class VTKRENDERINGWEBGPU_EXPORT VTK_MARSHALAUTO vtkWebGPUClearRGBPass : public vtkClearRGBPass
{
public:
  static vtkWebGPUClearRGBPass* New();
  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkWebGPUClearRGBPass, vtkClearRGBPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state s.
   */
  void Render(const vtkRenderState* s) override;

protected:
  vtkWebGPUClearRGBPass();
  ~vtkWebGPUClearRGBPass() override;

private:
  vtkWebGPUClearRGBPass(const vtkWebGPUClearRGBPass&) = delete;
  void operator=(const vtkWebGPUClearRGBPass&) = delete;
};

#define vtkWebGPUClearRGBPass_OVERRIDE_ATTRIBUTES vtkWebGPUClearRGBPass::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
