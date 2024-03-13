// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMapper2D
 * @brief   abstract class specifies interface for objects which render 2D actors
 *
 * vtkMapper2D is an abstract class which defines the interface for objects
 * which render two dimensional actors (vtkActor2D).
 *
 * @sa
 * vtkActor2D
 */

#ifndef vtkMapper2D_h
#define vtkMapper2D_h

#include "vtkAbstractMapper.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkViewport;
class vtkActor2D;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkMapper2D : public vtkAbstractMapper
{
public:
  vtkTypeMacro(vtkMapper2D, vtkAbstractMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual void RenderOverlay(vtkViewport*, vtkActor2D*) {}
  virtual void RenderOpaqueGeometry(vtkViewport*, vtkActor2D*) {}
  virtual void RenderTranslucentPolygonalGeometry(vtkViewport*, vtkActor2D*) {}
  virtual vtkTypeBool HasTranslucentPolygonalGeometry() { return 0; }

protected:
  vtkMapper2D() = default;
  ~vtkMapper2D() override = default;

private:
  vtkMapper2D(const vtkMapper2D&) = delete;
  void operator=(const vtkMapper2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
