// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkFixedPointVolumeRayCastCompositeShadeHelper
 * @brief   A helper that generates composite images for the volume ray cast mapper
 *
 * This is one of the helper classes for the vtkFixedPointVolumeRayCastMapper.
 * It will generate composite images using an alpha blending operation.
 * This class should not be used directly, it is a helper class for
 * the mapper and has no user-level API.
 *
 * @sa
 * vtkFixedPointVolumeRayCastMapper
 */

#ifndef vtkFixedPointVolumeRayCastCompositeShadeHelper_h
#define vtkFixedPointVolumeRayCastCompositeShadeHelper_h

#include "vtkFixedPointVolumeRayCastHelper.h"
#include "vtkRenderingVolumeModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkFixedPointVolumeRayCastMapper;
class vtkVolume;

class VTKRENDERINGVOLUME_EXPORT vtkFixedPointVolumeRayCastCompositeShadeHelper
  : public vtkFixedPointVolumeRayCastHelper
{
public:
  static vtkFixedPointVolumeRayCastCompositeShadeHelper* New();
  vtkTypeMacro(vtkFixedPointVolumeRayCastCompositeShadeHelper, vtkFixedPointVolumeRayCastHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void GenerateImage(int threadID, int threadCount, vtkVolume* vol,
    vtkFixedPointVolumeRayCastMapper* mapper) override;

protected:
  vtkFixedPointVolumeRayCastCompositeShadeHelper();
  ~vtkFixedPointVolumeRayCastCompositeShadeHelper() override;

private:
  vtkFixedPointVolumeRayCastCompositeShadeHelper(
    const vtkFixedPointVolumeRayCastCompositeShadeHelper&) = delete;
  void operator=(const vtkFixedPointVolumeRayCastCompositeShadeHelper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
