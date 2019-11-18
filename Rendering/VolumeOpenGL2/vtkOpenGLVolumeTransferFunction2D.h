/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeTransferFunction2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkOpenGLTransferFunction2D_h
#define vtkOpenGLTransferFunction2D_h
#ifndef __VTK_WRAP__

#include "vtkOpenGLVolumeLookupTable.h"

#include "vtkNew.h"

// Forward declarations
class vtkOpenGLRenderWindow;
class vtkImageResize;

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
class vtkOpenGLVolumeTransferFunction2D : public vtkOpenGLVolumeLookupTable
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
  vtkOpenGLVolumeTransferFunction2D& operator=(const vtkOpenGLVolumeTransferFunction2D&) = delete;
};

#endif // __VTK_WRAP__
#endif // vtkOpenGLTransferFunction2D_h
// VTK-HeaderTest-Exclude: vtkOpenGLVolumeTransferFunction2D.h
