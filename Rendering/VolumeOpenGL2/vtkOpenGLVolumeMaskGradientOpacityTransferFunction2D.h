/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D_h
#define vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D_h
#ifndef __VTK_WRAP__

#include "vtkOpenGLVolumeLookupTable.h"

#include "vtkNew.h"

// Forward declarations
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
class vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D : public vtkOpenGLVolumeLookupTable
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
  vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D(const vtkOpenGLVolumeLookupTable&) = delete;
  vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D& operator=(
    const vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D&) = delete;
};

#endif // __VTK_WRAP__
#endif // vtkOpenGLVolumeMaskTransferFunction2D_h
// VTK-HeaderTest-Exclude: vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D.h
