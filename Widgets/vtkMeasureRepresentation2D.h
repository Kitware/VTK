/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMeasureRepresentation2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMeasureRepresentation2D - represent the vtkMeasureWidget
// .SECTION Description
// The vtkMeasureRepresentation2D is a representation for the
// vtkMeasureWidget. This representation consists of a measuring line (axis)
// and two vtkHandleWidgets to place the end points of the line. Note that
// this particular widget draws its representation in the overlay plane.

// .SECTION See Also
// vtkMeasureWidget vtkMeasureRepresentation


#ifndef __vtkMeasureRepresentation2D_h
#define __vtkMeasureRepresentation2D_h

#include "vtkMeasureRepresentation.h"

class vtkAxisActor2D;
class vtkProperty2D;


class VTK_WIDGETS_EXPORT vtkMeasureRepresentation2D : public vtkMeasureRepresentation
{
public:
  // Description:
  // Instantiate class.
  static vtkMeasureRepresentation2D *New();

  // Description:
  // Standard VTK methods.
  vtkTypeRevisionMacro(vtkMeasureRepresentation2D,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Methods to Set/Get the coordinates of the two points defining
  // this representation. Note that methods are available for both
  // display and world coordinates.
  void GetPoint1WorldPosition(double pos[3]);
  void GetPoint2WorldPosition(double pos[3]);
  void SetPoint1DisplayPosition(double pos[3]);
  void SetPoint2DisplayPosition(double pos[3]);
  void GetPoint1DisplayPosition(double pos[3]);
  void GetPoint2DisplayPosition(double pos[3]);

  // Description:
  // Retrieve the vtkAxisActor2D used to draw the measurement axis. 
  vtkAxisActor2D *GetAxis();

  // Description:
  // Methods required by vtkProp superclass.
  virtual void ReleaseGraphicsResources(vtkWindow *w);
  virtual int RenderOverlay(vtkViewport *viewport);
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);

protected:
  vtkMeasureRepresentation2D();
  ~vtkMeasureRepresentation2D();

  // Add a line to the mix
  vtkAxisActor2D *AxisActor;
  vtkProperty2D  *AxisProperty;

private:
  vtkMeasureRepresentation2D(const vtkMeasureRepresentation2D&);  //Not implemented
  void operator=(const vtkMeasureRepresentation2D&);  //Not implemented
};

#endif
