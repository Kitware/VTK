// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUCameraPass
 * @brief   WebGPU camera pass.
 *
 * vtkWebGPUCameraPass is the WebGPU implementation of vtkCameraPass.
 *
 * @sa
 * vtkCameraPass
 */

#ifndef vtkWebGPUCameraPass_h
#define vtkWebGPUCameraPass_h

#include "vtkCameraPass.h"
#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkWrappingHints.h"         // For VTK_MARSHALAUTO

#include <tuple> // for std::tuple

VTK_ABI_NAMESPACE_BEGIN
class vtkOverrideAttribute;

class VTKRENDERINGWEBGPU_EXPORT VTK_MARSHALAUTO vtkWebGPUCameraPass : public vtkCameraPass
{
public:
  static vtkWebGPUCameraPass* New();
  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkWebGPUCameraPass, vtkCameraPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState* s) override;

protected:
  vtkWebGPUCameraPass();
  ~vtkWebGPUCameraPass() override;

private:
  vtkWebGPUCameraPass(const vtkWebGPUCameraPass&) = delete;
  void operator=(const vtkWebGPUCameraPass&) = delete;

  std::tuple<int, int, int, int> ComputeYInvertedViewport(vtkRenderer* renderer);
};

#define vtkWebGPUCameraPass_OVERRIDE_ATTRIBUTES vtkWebGPUCameraPass::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
