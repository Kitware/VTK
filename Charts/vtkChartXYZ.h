/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartXYZ.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkChartXYZ - Factory class for drawing 3D XYZ charts.
//
// .SECTION Description

#ifndef __vtkChartXYZ_h
#define __vtkChartXYZ_h

#include "vtkContextItem.h"
#include "vtkRect.h"        // For vtkRectf ivars
#include "vtkNew.h"         // For ivars

class vtkAnnotationLink;
class vtkAxis;
class vtkPlot;
class vtkTable;
class vtkTransform;
class vtkPen;

class VTK_CHARTS_EXPORT vtkChartXYZ : public vtkContextItem
{
public:
  vtkTypeMacro(vtkChartXYZ, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkChartXYZ * New();

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Add a plot to the chart, defaults to using the name of the y column
  virtual vtkPlot* AddPlot(int type);

  // Description:
  // Set the input for the chart, this should be done in the plot, but keeping
  // things simple while I get everything working...
  virtual void SetInput(vtkTable *input, const vtkStdString &x,
                        const vtkStdString &y, const vtkStdString &z);

  // Description:
  // Set the vtkAnnotationLink for the chart.
  virtual void SetAnnotationLink(vtkAnnotationLink *link);

  // Description:
  // Get the x (0), y (1) or z (2) axis.
  vtkAxis * GetAxis(int axis);

  void RecalculateTransform();

  void RecalculateBounds();

  // Description:
  // Set the geometry in pixel coordinates (origin and width/height).
  void SetGeometry(const vtkRectf &bounds);

  void SetAngle(double angle);
  void SetAroundX(bool isX);

protected:
  vtkChartXYZ();
  ~vtkChartXYZ();

  // Description:
  // Given the x, y and z vtkAxis, and a transform, calculate the transform that
  // the points in a chart would need to be drawn within the axes. This assumes
  // that the axes have the correct start and end positions, and that they are
  // perpendicular.
  bool CalculatePlotTransform(vtkAxis *x, vtkAxis *y, vtkAxis *z,
                              vtkTransform *transform);

  vtkRectf Geometry;

  vtkNew<vtkPen> Pen;
  vtkNew<vtkPen> AxisPen;

  class Private;
  Private *d;

private:
  vtkChartXYZ(const vtkChartXYZ &);    // Not implemented.
  void operator=(const vtkChartXYZ &); // Not implemented.
};

#endif
