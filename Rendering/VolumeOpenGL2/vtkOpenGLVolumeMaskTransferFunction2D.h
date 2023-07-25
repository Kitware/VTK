// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkOpenGLVolumeMaskTransferFunction2D_h
#define vtkOpenGLVolumeMaskTransferFunction2D_h

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
class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkOpenGLVolumeMaskTransferFunction2D
  : public vtkOpenGLVolumeLookupTable
{
public:
  vtkTypeMacro(vtkOpenGLVolumeMaskTransferFunction2D, vtkOpenGLVolumeLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkOpenGLVolumeMaskTransferFunction2D* New();

protected:
  vtkOpenGLVolumeMaskTransferFunction2D();

  /**
   * Update the internal texture object using the 2D image data
   */
  void InternalUpdate(vtkObject* func, int blendMode, double sampleDistance, double unitDistance,
    int filterValue) override;

  /**
   * Compute the ideal texture size based on the number of labels and transfer
   * functions in the label map.
   */
  void ComputeIdealTextureSize(
    vtkObject* func, int& width, int& height, vtkOpenGLRenderWindow* renWin) override;

private:
  vtkOpenGLVolumeMaskTransferFunction2D(const vtkOpenGLVolumeMaskTransferFunction2D&) = delete;
  void operator=(const vtkOpenGLVolumeMaskTransferFunction2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLVolumeMaskTransferFunction2D_h
