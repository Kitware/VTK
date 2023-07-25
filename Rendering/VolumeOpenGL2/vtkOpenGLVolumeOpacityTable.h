// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkOpenGLVolumeOpacityTable_h
#define vtkOpenGLVolumeOpacityTable_h

#include "vtkOpenGLVolumeLookupTable.h"
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro

#include "vtkVolumeMapper.h" // for vtkVolumeMapper

// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;

//----------------------------------------------------------------------------
class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkOpenGLVolumeOpacityTable
  : public vtkOpenGLVolumeLookupTable
{
public:
  vtkTypeMacro(vtkOpenGLVolumeOpacityTable, vtkOpenGLVolumeLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkOpenGLVolumeOpacityTable* New();

protected:
  vtkOpenGLVolumeOpacityTable() = default;

  /**
   * Update the internal texture object using the opacity transfer function
   */
  void InternalUpdate(vtkObject* func, int blendMode, double sampleDistance, double unitDistance,
    int filterValue) override;

  /**
   * Test whether the internal function needs to be updated.
   */
  bool NeedsUpdate(
    vtkObject* func, double scalarRange[2], int blendMode, double sampleDistance) override;

  int LastBlendMode = vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND;
  double LastSampleDistance = 1.0;

private:
  vtkOpenGLVolumeOpacityTable(const vtkOpenGLVolumeOpacityTable&) = delete;
  void operator=(const vtkOpenGLVolumeOpacityTable&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLVolumeOpacityTable_h
