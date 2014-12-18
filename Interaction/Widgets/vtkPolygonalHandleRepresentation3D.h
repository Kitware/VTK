/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygonalHandleRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolygonalHandleRepresentation3D - represent a user defined handle geometry in 3D space
// .SECTION Description
// This class serves as the geometrical representation of a vtkHandleWidget.
// The handle can be represented by an arbitrary polygonal data (vtkPolyData),
// set via SetHandle(vtkPolyData *). The actual position of the handle
// will be initially assumed to be (0,0,0). You can specify an offset from
// this position if desired.
// .SECTION See Also
// vtkPointHandleRepresentation3D vtkHandleRepresentation vtkHandleWidget


#ifndef vtkPolygonalHandleRepresentation3D_h
#define vtkPolygonalHandleRepresentation3D_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractPolygonalHandleRepresentation3D.h"

class VTKINTERACTIONWIDGETS_EXPORT vtkPolygonalHandleRepresentation3D
                : public vtkAbstractPolygonalHandleRepresentation3D
{
public:
  // Description:
  // Instantiate this class.
  static vtkPolygonalHandleRepresentation3D *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkPolygonalHandleRepresentation3D,
                       vtkAbstractPolygonalHandleRepresentation3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the position of the point in world and display coordinates.
  virtual void SetWorldPosition(double p[3]);

  // Description:
  // Set/get the offset of the handle position with respect to the handle
  // center, assumed to be the origin.
  vtkSetVector3Macro( Offset, double );
  vtkGetVector3Macro( Offset, double );

protected:
  vtkPolygonalHandleRepresentation3D();
  ~vtkPolygonalHandleRepresentation3D() {}

  double Offset[3];

private:
  vtkPolygonalHandleRepresentation3D(const vtkPolygonalHandleRepresentation3D&);  //Not implemented
  void operator=(const vtkPolygonalHandleRepresentation3D&);  //Not implemented
};

#endif
