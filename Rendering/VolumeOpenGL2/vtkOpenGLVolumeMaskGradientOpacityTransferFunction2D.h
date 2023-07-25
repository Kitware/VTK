// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D_h
#define vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D_h

#include "vtkOpenGLVolumeLookupTable.h"
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro

// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;

/**
 * \brief 2D Transfer function container for label map mask gradient opacity.
 *
 * Manages the texture fetched by the fragment shader when TransferFunction2D
 * mode is active. Update() assumes the vtkImageData instance used as source
 * is of type VTK_FLOAT and has 1 component (vtkVolumeProperty ensures this
 * is the case when the function is set).
 *
 * \sa vtkVolumeProperty::SetLabelGradientOpacity
 */
class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D
  : public vtkOpenGLVolumeLookupTable
{
public:
  vtkTypeMacro(vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D, vtkOpenGLVolumeLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D* New();

protected:
  vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D();

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
  vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D(
    const vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D&) = delete;
  void operator=(const vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLVolumeMaskTransferFunction2D_h
