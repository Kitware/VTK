// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class    vtkOpenGLCellGridMapper
 * @brief    CellGrid mapper using OpenGL to render exotic finite element fields and cells.
 */

#ifndef vtkOpenGLCellGridMapper_h
#define vtkOpenGLCellGridMapper_h

#include "vtkCellGridMapper.h"
#include "vtkRenderingCellGridModule.h" // For export macro
// #include "vtkCellGridRenderRequest.h" // For RenderQuery ivar
#include <memory> // for ivar

VTK_ABI_NAMESPACE_BEGIN

class vtkGenericOpenGLResourceFreeCallback;

class VTKRENDERINGCELLGRID_EXPORT vtkOpenGLCellGridMapper : public vtkCellGridMapper
{
public:
  static vtkOpenGLCellGridMapper* New();
  vtkTypeMacro(vtkOpenGLCellGridMapper, vtkCellGridMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  bool GetSupportsSelection() override { return true; }

  /**
   * Make a shallow copy of this mapper.
   */
  void ShallowCopy(vtkAbstractMapper* m) override;

protected:
  vtkOpenGLCellGridMapper();
  ~vtkOpenGLCellGridMapper() override;

  vtkGenericOpenGLResourceFreeCallback* ResourceCallback;

private:
  vtkOpenGLCellGridMapper(const vtkOpenGLCellGridMapper&) = delete;
  void operator=(const vtkOpenGLCellGridMapper&) = delete;

  class vtkInternals;
  vtkInternals* Internal;
  // vtkNew<vtkCellGridRenderRequest> RenderQuery;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLCellGridMapper_h
