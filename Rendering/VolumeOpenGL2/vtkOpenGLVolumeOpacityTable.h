/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeOpacityTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkOpenGLVolumeOpacityTable_h
#define vtkOpenGLVolumeOpacityTable_h

#include "vtkOpenGLVolumeLookupTable.h"
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro

#include "vtkVolumeMapper.h" // for vtkVolumeMapper

// Forward declarations
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

#endif // vtkOpenGLVolumeOpacityTable_h
