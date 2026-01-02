// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkWebGPUGlyph3DMapper
 * @brief Generate 3D glyphs at points in input dataset using webgpu
 *
 */

#ifndef vtkWebGPUGlyph3DMapper_h
#define vtkWebGPUGlyph3DMapper_h

#include "vtkGlyph3DMapper.h"

#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtkWrappingHints.h"         // For VTK_MARSHALAUTO

#include <memory> // for unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkOverrideAttribute;
class VTKRENDERINGWEBGPU_EXPORT VTK_MARSHALAUTO vtkWebGPUGlyph3DMapper : public vtkGlyph3DMapper
{
public:
  static vtkWebGPUGlyph3DMapper* New();
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkWebGPUGlyph3DMapper, vtkGlyph3DMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Render(vtkRenderer* renderer, vtkActor* actor) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow* window) override;

protected:
  vtkWebGPUGlyph3DMapper();
  ~vtkWebGPUGlyph3DMapper() override;

private:
  vtkWebGPUGlyph3DMapper(const vtkWebGPUGlyph3DMapper&) = delete;
  void operator=(const vtkWebGPUGlyph3DMapper&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#define vtkWebGPUGlyph3DMapper_OVERRIDE_ATTRIBUTES                                                 \
  vtkWebGPUGlyph3DMapper::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END

#endif
