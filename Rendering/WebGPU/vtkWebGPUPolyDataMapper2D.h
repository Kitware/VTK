// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUPolyDataMapper2D
 * @brief   2D PolyData support for WebGPU
 *
 * vtkWebGPUPolyDataMapper2D provides 2D PolyData annotation support for
 * vtk under WebGPU. Normally the user should use vtkPolyDataMapper2D
 * which in turn will use this class.
 *
 * @sa
 * vtkPolyDataMapper2D
 */

#ifndef vtkWebGPUPolyDataMapper2D_h
#define vtkWebGPUPolyDataMapper2D_h

#include "vtkPolyDataMapper2D.h"
#include "vtkRenderingWebGPUModule.h" // For export macro

#include <memory> // for unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkActor2D;
class vtkPoints;
class vtkWebGPUPolyDataMapper2DInternals;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUPolyDataMapper2D : public vtkPolyDataMapper2D
{
public:
  vtkTypeMacro(vtkWebGPUPolyDataMapper2D, vtkPolyDataMapper2D);
  static vtkWebGPUPolyDataMapper2D* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Actually draw the poly data.
   */
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkWebGPUPolyDataMapper2D();
  ~vtkWebGPUPolyDataMapper2D() override;

private:
  vtkWebGPUPolyDataMapper2D(const vtkWebGPUPolyDataMapper2D&) = delete;
  void operator=(const vtkWebGPUPolyDataMapper2D&) = delete;

  friend class vtkWebGPUPolyDataMapper2DInternals;
  std::unique_ptr<vtkWebGPUPolyDataMapper2DInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
