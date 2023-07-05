// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkOpenGLVolumeTransferFunction2D_h
#define vtkOpenGLVolumeTransferFunction2D_h

#include "vtkImageResize.h" // for vtkImageResize
#include "vtkOpenGLVolumeLookupTable.h"
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro

#include "vtkNew.h" // for vtkNew

// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;

/**
 * \brief 2D Transfer function container.
 *
 * Manages the texture fetched by the fragment shader when TransferFunction2D
 * mode is active. Update() assumes the vtkImageData instance used as source
 * is of type VTK_FLOAT and has 4 components (vtkVolumeProperty ensures this
 * is the case when the function is set).
 *
 * \sa vtkVolumeProperty::SetTransferFunction2D
 */
class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkOpenGLVolumeTransferFunction2D
  : public vtkOpenGLVolumeLookupTable
{
public:
  vtkTypeMacro(vtkOpenGLVolumeTransferFunction2D, vtkOpenGLVolumeLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkOpenGLVolumeTransferFunction2D* New();

protected:
  vtkOpenGLVolumeTransferFunction2D();

  /**
   * Update the internal texture object using the 2D image data
   */
  void InternalUpdate(vtkObject* func, int blendMode, double sampleDistance, double unitDistance,
    int filterValue) override;

  /**
   * Override needs update to not test for scalar range changes since the range
   * is encoded in the vtkImageData
   */
  bool NeedsUpdate(
    vtkObject* func, double scalarRange[2], int blendMode, double sampleDistance) override;

  /**
   * Override allocate table to do nothing as no internal table management is
   * needed.
   */
  void AllocateTable() override;

  vtkNew<vtkImageResize> ResizeFilter;

private:
  vtkOpenGLVolumeTransferFunction2D(const vtkOpenGLVolumeTransferFunction2D&) = delete;
  void operator=(const vtkOpenGLVolumeTransferFunction2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLTransferFunction2D_h
