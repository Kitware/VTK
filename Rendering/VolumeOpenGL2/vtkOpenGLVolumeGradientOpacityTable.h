/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeGradientOpacityTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkOpenGLVolumeGradientOpacityTable_h
#define vtkOpenGLVolumeGradientOpacityTable_h

#include "vtkOpenGLVolumeLookupTable.h"
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro

// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;

//----------------------------------------------------------------------------
class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkOpenGLVolumeGradientOpacityTable
  : public vtkOpenGLVolumeLookupTable
{
public:
  vtkTypeMacro(vtkOpenGLVolumeGradientOpacityTable, vtkOpenGLVolumeLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkOpenGLVolumeGradientOpacityTable* New();

protected:
  vtkOpenGLVolumeGradientOpacityTable() = default;

  /**
   * Update the internal texture object using the gradient opacity transfer
   * function
   */
  void InternalUpdate(vtkObject* func, int blendMode, double sampleDistance, double unitDistance,
    int filterValue) override;

private:
  vtkOpenGLVolumeGradientOpacityTable(const vtkOpenGLVolumeGradientOpacityTable&) = delete;
  void operator=(const vtkOpenGLVolumeGradientOpacityTable&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLVolumeGradientOpacityTable_h
