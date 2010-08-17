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

  virtual void GetBounds(double bounds[4]);
  virtual bool Paint(vtkContext2D *painter);

  void SelectPoint(double* currentPoint);
  void SelectPoint(vtkIdType pointId);

  void DeselectPoint(double* currentPoint);
  void DeselectPoint(vtkIdType pointId);
  void DeselectAllPoints();

  void ToggleSelectPoint(double* currentPoint);
  void ToggleSelectPoint(vtkIdType pointId);
  // if tolerance is -1, then use the ItemPointSize as a tolerance
  vtkIdType GetPointId(double* pos, double tolerance = -1.);

protected:
  vtkControlPointsItem();
  virtual ~vtkControlPointsItem();

  static void CallComputePoints(vtkObject* sender, unsigned long event, void* receiver, void* params);
  virtual void ComputePoints();
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
