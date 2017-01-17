/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrientedPolygonalHandleRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOrientedPolygonalHandleRepresentation3D
 * @brief   represent a user defined handle geometry in 3D while maintaining a fixed orientation w.r.t the camera.
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

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractPolygonalHandleRepresentation3D.h"

class VTKINTERACTIONWIDGETS_EXPORT vtkOrientedPolygonalHandleRepresentation3D
                : public vtkAbstractPolygonalHandleRepresentation3D
{
public:
  /**
   * Instantiate this class.
   */
  static vtkOrientedPolygonalHandleRepresentation3D *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkOrientedPolygonalHandleRepresentation3D,
                       vtkAbstractPolygonalHandleRepresentation3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

protected:
  vtkOrientedPolygonalHandleRepresentation3D();
  ~vtkOrientedPolygonalHandleRepresentation3D() VTK_OVERRIDE;

  /**
   * Override the superclass method.
   */
  void UpdateHandle() VTK_OVERRIDE;

private:
  vtkOrientedPolygonalHandleRepresentation3D(const vtkOrientedPolygonalHandleRepresentation3D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOrientedPolygonalHandleRepresentation3D&) VTK_DELETE_FUNCTION;
};

#endif

