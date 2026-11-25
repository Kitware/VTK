// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUToneMappingPass
 * @brief   WebGPU implementation of vtkToneMappingPass.
 *
 * A WebGPU render pipeline is created for the format of the color attachment it draws into, and
 * the renderer creates them before it records any command. So the renderer runs its render passes
 * once before that, and this pass then makes it draw its geometry into a 16 bit floating point
 * color texture. Once the renderer has recorded its commands, the pass draws that texture into the
 * color attachment of the render window with the selected operator, in the viewport of the
 * renderer. Unlike the OpenGL implementation, this tone maps everything the renderer draws, not
 * only what the delegate draws.
 *
 * @sa
 * vtkToneMappingPass
 */

#ifndef vtkWebGPUToneMappingPass_h
#define vtkWebGPUToneMappingPass_h

#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkToneMappingPass.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO
#include "vtk_wgpu.h"         // for webgpu C API

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkOverrideAttribute;
class vtkWebGPURenderer;

class VTKRENDERINGWEBGPU_EXPORT VTK_MARSHALAUTO vtkWebGPUToneMappingPass : public vtkToneMappingPass
{
public:
  static vtkWebGPUToneMappingPass* New();
  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkWebGPUToneMappingPass, vtkToneMappingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state.
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own resources.
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  /**
   * Get the format of the color texture that the renderer draws into.
   */
  WGPUTextureFormat GetColorTextureFormat() const;

  /**
   * Get the color texture that the renderer draws into. It is (re)created at the size of the
   * render window of the renderer.
   */
  WGPUTextureView GetColorTextureView(vtkWebGPURenderer* renderer);

  /**
   * Tone map the color texture into the color attachment of the render window, in the viewport of
   * the renderer. The renderer calls this once it has recorded its commands.
   */
  void Resolve(vtkWebGPURenderer* renderer);

protected:
  vtkWebGPUToneMappingPass();
  ~vtkWebGPUToneMappingPass() override;

private:
  vtkWebGPUToneMappingPass(const vtkWebGPUToneMappingPass&) = delete;
  void operator=(const vtkWebGPUToneMappingPass&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#define vtkWebGPUToneMappingPass_OVERRIDE_ATTRIBUTES                                               \
  vtkWebGPUToneMappingPass::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
