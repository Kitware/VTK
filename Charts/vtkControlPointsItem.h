/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkControlPointsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkControlPointsItem - Abstract class for control points items.
// .SECTION Description
// vtkControlPointsItem provides control point painting and management for
// subclasses that provide points (typically control points of a transfer
// function)
// .SECTION See Also
// vtkScalarsToColorsItem
// vtkPiecewiseControlPointsItem

#ifndef __vtkControlPointsItem_h
#define __vtkControlPointsItem_h

#include "vtkPlot.h"

class vtkCallbackCommand;
class vtkContext2D;
class vtkPoints2D;

class VTK_CHARTS_EXPORT vtkControlPointsItem: public vtkPlot
{
public:
  vtkTypeMacro(vtkControlPointsItem, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Bounds of the item, typically the bound of all the control points
  virtual void GetBounds(double bounds[4]);

  // Decription:
  // Paint the points with a fixed size (cosmetic) which doesn't depend
  // on the scene zoom factor. Selected and unselected points are drawn
  // with a different color.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Select a point by its ID
  void SelectPoint(vtkIdType pointId);

  // Description:
  // Utility function that selects a point providing its coordinates.
  // To be found, the position of the point must be no further away than its
  // painted point size
  void SelectPoint(double* currentPoint);

  // Description:
  // Unselect a point by its ID
  void DeselectPoint(vtkIdType pointId);

  // Description:
  // Utility function that unselects a point providing its coordinates.
  // To be found, the position of the point must be no further away than its
  // painted point size
  void DeselectPoint(double* currentPoint);

  // Description:
  // Unselect all the previously selected points
  void DeselectAllPoints();

  // Description:
  // Toggle the selection of a point by its ID. If the point was selected then
  // unselect it, otherwise select it.
  void ToggleSelectPoint(vtkIdType pointId);

  // Description:
  // Utility function that toggles the selection a point providing its
  // coordinates. To be found, the position of the point must be no further
  // away than its painted point size
  void ToggleSelectPoint(double* currentPoint);

  // Description:
  // Return the vtkIdType of the point given its coordinates and a tolerance.
  // If tolerance is -1, to be found, the position of the point must be no
  // further away than its painted point size (ItemPointSize)
  vtkIdType GetPointId(double* pos, double tolerance = -1.);

protected:
  vtkControlPointsItem();
  virtual ~vtkControlPointsItem();

  static void CallComputePoints(vtkObject* sender, unsigned long event, void* receiver, void* params);

  // Decription:
  // Must be reimplemented by subclasses to calculate the points to draw.
  // It's subclass responsibility to call ComputePoints() via the callback
  virtual void ComputePoints();

  // Description:
  // Internal function that paints a collection of points and optionally
  // excludes some.
  void DrawPoints(vtkContext2D* painter, vtkPoints2D* points, vtkIdTypeArray* excludePoints = 0);

  vtkPoints2D*        Points;
  vtkPoints2D*        SelectedPoints;
  vtkCallbackCommand* Callback;

  float               ScreenPointRadius;
  float               ItemPointRadius2;
private:
  vtkControlPointsItem(const vtkControlPointsItem &); // Not implemented.
  void operator=(const vtkControlPointsItem &);   // Not implemented.
};

#endif
