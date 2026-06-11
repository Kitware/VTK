// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUBatchedLabeledDataMapper
 * @brief   WebGPU backend for vtkBatchedLabeledDataMapper
 *
 * WebGPU implementation of vtkBatchedLabeledDataMapper. Renders all labels in a
 * single batched draw call using a glyph atlas texture and instanced rendering.
 * Because WebGPU does not support geometry shaders, the point-to-quad expansion
 * is implemented in the vertex shader using instanced rendering
 * (18 vertices per label instance, 3 layers × 6 vertices).
 *
 * It should be noted that
 * 1. layer 0 is the color of the label from atlas texture
 * 2. layer 1 is the background color
 * 3. layer 2 is the frame color
 *
 * @note Unlike OpenGL (which uploads immediately in UploadGlyphAtlas), the WebGPU upload
 * is deferred to RenderOpaqueGeometry because a vtkWebGPURenderWindow is required
 * to create the GPU texture, and it is not available at BuildLabels time.
 *
 * @sa
 * vtkBatchedLabeledDataMapper
 */

#ifndef vtkWebGPUBatchedLabeledDataMapper_h
#define vtkWebGPUBatchedLabeledDataMapper_h

#include "vtkBatchedLabeledDataMapper.h"
#include "vtkNew.h"                   // For vtkNew
#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkWrappingHints.h"         // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkOverrideAttribute;
class vtkWebGPUBatchedLabeledDataMapperInternals;

class VTKRENDERINGWEBGPU_EXPORT VTK_MARSHALAUTO vtkWebGPUBatchedLabeledDataMapper
  : public vtkBatchedLabeledDataMapper
{
public:
  static vtkWebGPUBatchedLabeledDataMapper* New();
  vtkTypeMacro(vtkWebGPUBatchedLabeledDataMapper, vtkBatchedLabeledDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkOverrideAttribute* CreateOverrideAttributes();

  using vtkBatchedLabeledDataMapper::SetLabelTextProperty;
  void SetLabelTextProperty(vtkTextProperty* p, int type) override;

  void RenderOpaqueGeometry(vtkViewport* viewport, vtkActor2D* actor) override;
  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkWebGPUBatchedLabeledDataMapper();
  ~vtkWebGPUBatchedLabeledDataMapper() override;

  void ActivateGlyphTexture() override {}
  void DeactivateGlyphTexture() override {}

private:
  vtkWebGPUBatchedLabeledDataMapper(const vtkWebGPUBatchedLabeledDataMapper&) = delete;
  void operator=(const vtkWebGPUBatchedLabeledDataMapper&) = delete;

  vtkNew<vtkWebGPUBatchedLabeledDataMapperInternals> Helper;
  vtkNew<vtkActor> DummyActor;

  friend class vtkWebGPUBatchedLabeledDataMapperInternals;

  void SetupHelper();
  bool HelperSetup = false;
};

#define vtkWebGPUBatchedLabeledDataMapper_OVERRIDE_ATTRIBUTES                                      \
  vtkWebGPUBatchedLabeledDataMapper::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END

#endif
