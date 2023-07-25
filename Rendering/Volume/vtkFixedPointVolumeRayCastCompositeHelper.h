// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkFixedPointVolumeRayCastCompositeHelper
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

#ifndef vtkFixedPointVolumeRayCastCompositeHelper_h
#define vtkFixedPointVolumeRayCastCompositeHelper_h

#include "vtkFixedPointVolumeRayCastHelper.h"
#include "vtkRenderingVolumeModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkFixedPointVolumeRayCastMapper;
class vtkVolume;

class VTKRENDERINGVOLUME_EXPORT vtkFixedPointVolumeRayCastCompositeHelper
  : public vtkFixedPointVolumeRayCastHelper
{
public:
  static vtkFixedPointVolumeRayCastCompositeHelper* New();
  vtkTypeMacro(vtkFixedPointVolumeRayCastCompositeHelper, vtkFixedPointVolumeRayCastHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void GenerateImage(int threadID, int threadCount, vtkVolume* vol,
    vtkFixedPointVolumeRayCastMapper* mapper) override;

protected:
  vtkFixedPointVolumeRayCastCompositeHelper();
  ~vtkFixedPointVolumeRayCastCompositeHelper() override;

private:
  vtkFixedPointVolumeRayCastCompositeHelper(
    const vtkFixedPointVolumeRayCastCompositeHelper&) = delete;
  void operator=(const vtkFixedPointVolumeRayCastCompositeHelper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
