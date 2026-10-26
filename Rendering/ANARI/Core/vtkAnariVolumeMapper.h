// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkAnariVolumeMapper_h
#define vtkAnariVolumeMapper_h

#include "vtkRenderingAnariCoreModule.h" // For export macro
#include "vtkVolumeMapper.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkOverrideAttribute;
class vtkRenderer;

/**
 * @class vtkAnariGPUVolumeRayCastMapper
 * @brief ANARI specific class for GPU volume rendering.
 *
 * This class is just a factory override specific to this module. As the ANARI rendering
 * implementation are in the view node, the node factory will detect this class signature to
 * instantiate vtkAnariVolumeMapperNode.
 * @sa vtkAnariVolumeMapperNode
 */
class VTKRENDERINGANARICORE_EXPORT vtkAnariVolumeMapper : public vtkVolumeMapper
{
public:
  static vtkAnariVolumeMapper* New();
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkAnariVolumeMapper, vtkVolumeMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkAnariVolumeMapper() = default;
  ~vtkAnariVolumeMapper() override = default;

  /**
   * Necessary overrides of the base class.
   * This function does not do anything.
   */
  void Render(vtkRenderer* vtkNotUsed(renderer), vtkVolume* vtkNotUsed(volume)) override {}

private:
  vtkAnariVolumeMapper(const vtkAnariVolumeMapper&) = delete;
  void operator=(const vtkAnariVolumeMapper&) = delete;
};

#define vtkAnariVolumeMapper_OVERRIDE_ATTRIBUTES vtkAnariVolumeMapper::CreateOverrideAttributes()

VTK_ABI_NAMESPACE_END
#endif
