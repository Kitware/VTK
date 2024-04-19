// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOrientedPolygonalHandleRepresentation3D
 * @brief   represent a user defined handle geometry in 3D while maintaining a fixed orientation
 * w.r.t the camera.
 *
 * This class serves as the geometrical representation of a vtkHandleWidget.
 * The handle can be represented by an arbitrary polygonal data (vtkPolyData),
 * set via SetHandle(vtkPolyData *). The actual position of the handle
 * will be initially assumed to be (0,0,0). You can specify an offset from
 * this position if desired. This class differs from
 * vtkPolygonalHandleRepresentation3D in that the handle will always remain
 * front facing, ie it maintains a fixed orientation with respect to the
 * camera. This is done by using vtkFollowers internally to render the actors.
 * @sa
 * vtkPolygonalHandleRepresentation3D vtkHandleRepresentation vtkHandleWidget
 */

#ifndef vtkOrientedPolygonalHandleRepresentation3D_h
#define vtkOrientedPolygonalHandleRepresentation3D_h

#include "vtkAbstractPolygonalHandleRepresentation3D.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONWIDGETS_EXPORT vtkOrientedPolygonalHandleRepresentation3D
  : public vtkAbstractPolygonalHandleRepresentation3D
{
public:
  /**
   * Instantiate this class.
   */
  static vtkOrientedPolygonalHandleRepresentation3D* New();

  ///@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(
    vtkOrientedPolygonalHandleRepresentation3D, vtkAbstractPolygonalHandleRepresentation3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

protected:
  vtkOrientedPolygonalHandleRepresentation3D();
  ~vtkOrientedPolygonalHandleRepresentation3D() override;

  /**
   * Override the superclass method.
   */
  void UpdateHandle() override;

private:
  vtkOrientedPolygonalHandleRepresentation3D(
    const vtkOrientedPolygonalHandleRepresentation3D&) = delete;
  void operator=(const vtkOrientedPolygonalHandleRepresentation3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
