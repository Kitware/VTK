// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUImageMapper
 * @brief   2D image display using webgpu
 *
 */

#ifndef vtkWebGPUImageMapper_h
#define vtkWebGPUImageMapper_h

#include "vtkImageMapper.h"
#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkWrappingHints.h"         // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOverrideAttribute;
class vtkTexturedActor2D;
class vtkWebGPURenderer;
class VTKRENDERINGWEBGPU_EXPORT VTK_MARSHALAUTO vtkWebGPUImageMapper : public vtkImageMapper
{
public:
  vtkTypeMacro(vtkWebGPUImageMapper, vtkImageMapper);
  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes();
  static vtkWebGPUImageMapper* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void ReleaseGraphicsResources(vtkWindow* window) override;

  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor) override;

  void RenderData(vtkViewport* viewport, vtkImageData* image, vtkActor2D* actor) override;

protected:
  vtkWebGPUImageMapper();
  ~vtkWebGPUImageMapper() override;

private:
  vtkWebGPUImageMapper(const vtkWebGPUImageMapper&) = delete;
  void operator=(const vtkWebGPUImageMapper&) = delete;

  vtkNew<vtkTexturedActor2D> ProxyActor;

  void CreateTextureFromImage(vtkImageData* input, vtkWebGPURenderer* wgpuRenderer);
};
#define vtkWebGPUImageMapper_OVERRIDE_ATTRIBUTES vtkWebGPUImageMapper::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
