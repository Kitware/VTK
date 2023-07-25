// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolygonalHandleRepresentation3D
 * @brief   represent a user defined handle geometry in 3D space
 *
 * This class serves as the geometrical representation of a vtkHandleWidget.
 * The handle can be represented by an arbitrary polygonal data (vtkPolyData),
 * set via SetHandle(vtkPolyData *). The actual position of the handle
 * will be initially assumed to be (0,0,0). You can specify an offset from
 * this position if desired.
 * @sa
 * vtkPointHandleRepresentation3D vtkHandleRepresentation vtkHandleWidget
 */

#ifndef vtkPolygonalHandleRepresentation3D_h
#define vtkPolygonalHandleRepresentation3D_h

#include "vtkAbstractPolygonalHandleRepresentation3D.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONWIDGETS_EXPORT vtkPolygonalHandleRepresentation3D
  : public vtkAbstractPolygonalHandleRepresentation3D
{
public:
  /**
   * Instantiate this class.
   */
  static vtkPolygonalHandleRepresentation3D* New();

  ///@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkPolygonalHandleRepresentation3D, vtkAbstractPolygonalHandleRepresentation3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Set the position of the point in world and display coordinates.
   */
  void SetWorldPosition(double p[3]) override;

  ///@{
  /**
   * Set/get the offset of the handle position with respect to the handle
   * center, assumed to be the origin.
   */
  vtkSetVector3Macro(Offset, double);
  vtkGetVector3Macro(Offset, double);
  ///@}

protected:
  vtkPolygonalHandleRepresentation3D();
  ~vtkPolygonalHandleRepresentation3D() override = default;

  double Offset[3];

private:
  vtkPolygonalHandleRepresentation3D(const vtkPolygonalHandleRepresentation3D&) = delete;
  void operator=(const vtkPolygonalHandleRepresentation3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
