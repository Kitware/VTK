// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkAnariGPUVolumeRayCastMapper_h
#define vtkAnariGPUVolumeRayCastMapper_h

#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkRenderingAnariCoreModule.h" // For export macro

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
class VTKRENDERINGANARICORE_EXPORT vtkAnariGPUVolumeRayCastMapper : public vtkGPUVolumeRayCastMapper
{
public:
  static vtkAnariGPUVolumeRayCastMapper* New();
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkAnariGPUVolumeRayCastMapper, vtkGPUVolumeRayCastMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Necessary override of the base class.
   * This function does not do anything.
   */
  void GetReductionRatio(double vtkNotUsed(ratio)[3]) override {}

protected:
  vtkAnariGPUVolumeRayCastMapper() = default;
  ~vtkAnariGPUVolumeRayCastMapper() override = default;

  ///@{
  /**
   * Necessary overrides of the base class.
   * This function does not do anything.
   */
  void PreRender(vtkRenderer* vtkNotUsed(ren), vtkVolume* vtkNotUsed(vol),
    double vtkNotUsed(datasetBounds)[6], double vtkNotUsed(scalarRange)[2],
    int vtkNotUsed(numberOfScalarComponents), unsigned int vtkNotUsed(numberOfLevels)) override
  {
  }
  void RenderBlock(vtkRenderer* vtkNotUsed(ren), vtkVolume* vtkNotUsed(vol),
    unsigned int vtkNotUsed(level)) override
  {
  }
  void PostRender(vtkRenderer* vtkNotUsed(ren), int vtkNotUsed(numberOfScalarComponents)) override
  {
  }
  ///@}

private:
  vtkAnariGPUVolumeRayCastMapper(const vtkAnariGPUVolumeRayCastMapper&) = delete;
  void operator=(const vtkAnariGPUVolumeRayCastMapper&) = delete;
};

#define vtkAnariGPUVolumeRayCastMapper_OVERRIDE_ATTRIBUTES                                         \
  vtkAnariGPUVolumeRayCastMapper::CreateOverrideAttributes()

VTK_ABI_NAMESPACE_END
#endif
