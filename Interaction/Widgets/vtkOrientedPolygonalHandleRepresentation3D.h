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
// .NAME vtkOrientedPolygonalHandleRepresentation3D - represent a user defined handle geometry in 3D while maintaining a fixed orientation w.r.t the camera.
// .SECTION Description
// This class serves as the geometrical representation of a vtkHandleWidget.
// The handle can be represented by an arbitrary polygonal data (vtkPolyData),
// set via SetHandle(vtkPolyData *). The actual position of the handle
// will be initially assumed to be (0,0,0). You can specify an offset from
// this position if desired. This class differs from
// vtkPolygonalHandleRepresentation3D in that the handle will always remain
// front facing, ie it maintains a fixed orientation with respect to the
// camera. This is done by using vtkFollowers internally to render the actors.
// .SECTION See Also
// vtkPolygonalHandleRepresentation3D vtkHandleRepresentation vtkHandleWidget


#ifndef __vtkOrientedPolygonalHandleRepresentation3D_h
#define __vtkOrientedPolygonalHandleRepresentation3D_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractPolygonalHandleRepresentation3D.h"

class VTKINTERACTIONWIDGETS_EXPORT vtkOrientedPolygonalHandleRepresentation3D
                : public vtkAbstractPolygonalHandleRepresentation3D
{
public:
  // Description:
  // Instantiate this class.
  static vtkOrientedPolygonalHandleRepresentation3D *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkOrientedPolygonalHandleRepresentation3D,
                       vtkAbstractPolygonalHandleRepresentation3D);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkOrientedPolygonalHandleRepresentation3D();
  ~vtkOrientedPolygonalHandleRepresentation3D();

  // Description:
  // Override the superclass method.
  virtual void UpdateHandle();

private:
  vtkOrientedPolygonalHandleRepresentation3D(const vtkOrientedPolygonalHandleRepresentation3D&);  //Not implemented
  void operator=(const vtkOrientedPolygonalHandleRepresentation3D&);  //Not implemented
};

#endif

