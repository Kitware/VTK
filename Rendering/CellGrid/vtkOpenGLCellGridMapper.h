/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class    vtkOpenGLCellGridMapper
 * @brief    CellGrid mapper using OpenGL to render exotic finite element fields and cells.
 */

#ifndef vtkOpenGLCellGridMapper_h
#define vtkOpenGLCellGridMapper_h

#include "vtkCellGridMapper.h"
#include "vtkRenderingCellGridModule.h" // For export macro
#include <memory>                       // for ivar

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGCELLGRID_EXPORT vtkOpenGLCellGridMapper : public vtkCellGridMapper
{
public:
  static vtkOpenGLCellGridMapper* New();
  vtkTypeMacro(vtkOpenGLCellGridMapper, vtkCellGridMapper);
  void PrintSelf(ostream&, vtkIndent indent) override;

  void Render(vtkRenderer*, vtkActor*) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow* window) override;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by vtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  bool GetSupportsSelection() override { return false; }

  /**
   * Make a shallow copy of this mapper.
   */
  void ShallowCopy(vtkAbstractMapper* m) override;

protected:
  vtkOpenGLCellGridMapper();
  ~vtkOpenGLCellGridMapper() override;

private:
  vtkOpenGLCellGridMapper(const vtkOpenGLCellGridMapper&) = delete;
  void operator=(const vtkOpenGLCellGridMapper&) = delete;

  class vtkInternals;
  vtkInternals* Internal;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLCellGridMapper_h
