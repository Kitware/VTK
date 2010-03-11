/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartParallelCoordinates.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkChartParallelCoordinates - Factory class for drawing 2D charts
//
// .SECTION Description
// This defines the interface for a chart.

#ifndef __vtkChartParallelCoordinates_h
#define __vtkChartParallelCoordinates_h

#include "vtkChart.h"

class vtkIdTypeArray;

class VTK_CHARTS_EXPORT vtkChartParallelCoordinates : public vtkChart
{
public:
  vtkTypeRevisionMacro(vtkChartParallelCoordinates, vtkChart);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a parallel coordinates chart
  static vtkChartParallelCoordinates* New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Add a plot to the chart, defaults to using the name of the y column
  virtual vtkPlot* AddPlot(int type);

  // Description:
  // Remove the plot at the specified index, returns true if successful,
  // false if the index was invalid.
  virtual bool RemovePlot(vtkIdType index);

  // Description:
  // Remove all plots from the chart.
  virtual void ClearPlots();

  // Description:
  // Get the plot at the specified index, returns null if the index is invalid.
  virtual vtkPlot* GetPlot(vtkIdType index);

  // Description:
  // Get the number of plots the chart contains.
  virtual vtkIdType GetNumberOfPlots();

  // Description:
  // Get the axis specified by axisIndex. 0 is x, 1 is y. This should probably
  // be improved either using a string or enum to select the axis.
  virtual vtkAxis* GetAxis(int axisIndex);

  // Description:
  // Get the number of axes in the current chart.
  virtual vtkIdType GetNumberOfAxes();

  // Description:
  // Request that the chart recalculates the range of its axes. Especially
  // useful in applications after the parameters of plots have been modified.
  virtual void RecalculateBounds();

//BTX
  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse enter event.
  virtual bool MouseEnterEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse leave event.
  virtual bool MouseLeaveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button down event
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button release event.
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse wheel event, positive delta indicates forward movement of the wheel.
  virtual bool MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta);
//ETX

//BTX
protected:
  vtkChartParallelCoordinates();
  ~vtkChartParallelCoordinates();

  // Description:
  // Private storage object - where we hide all of our STL objects...
  class Private;
  Private *Storage;

  bool GeometryValid;

  // Description:
  // Selected indices for the table the plot is rendering
  vtkIdTypeArray *Selection;

  void UpdateGeometry();
  void CalculatePlotTransform();

private:
  vtkChartParallelCoordinates(const vtkChartParallelCoordinates &); // Not implemented.
  void operator=(const vtkChartParallelCoordinates &);   // Not implemented.
//ETX
};

#endif //__vtkChartParallelCoordinates_h
