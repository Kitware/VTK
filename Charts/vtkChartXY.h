/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartXY.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkChartXY - Factory class for drawing XY charts
//
// .SECTION Description
// This class implements an XY chart.

#ifndef __vtkChartXY_h
#define __vtkChartXY_h

#include "vtkChart.h"

class vtkPlot;
class vtkAxis;
class vtkPlotGrid;
class vtkTable;
class vtkChartXYPrivate; // Private class to keep my STL vector in...

class VTK_CHARTS_EXPORT vtkChartXY : public vtkChart
{
public:
  vtkTypeRevisionMacro(vtkChartXY, vtkChart);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

//BTX
  // Description:
  // Enum of the available chart types
  enum Type {
    LINE,
    STACKED};
//ETX

  // Description:
  // Creates a 2D Chart object.
  static vtkChartXY *New();

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

//BTX
  // Description:
  // Add a plot to the chart, defaults to using the name of the y column
  virtual vtkPlot * AddPlot(vtkChart::Type type);
//ETX

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
  virtual vtkIdType GetNumberPlots();

  // Description:
  // Request that the chart recalculates the range of its axes. Especially
  // useful in applications after the parameters of plots have been modified.
  virtual void RecalculateBounds();

  // Description:
  // If true then the axes will be drawn at the origin (scientific style).
  vtkSetMacro(DrawAxesAtOrigin, bool);
  vtkGetMacro(DrawAxesAtOrigin, bool);

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
  vtkChartXY();
  ~vtkChartXY();

  // Description:
  // Recalculate the plot area transform to fit in all points that will be
  // plotted.
  void RecalculatePlotTransform();

  // Description:
  // Calculate the optimal zoom level such that all of the points to be plotted
  // will fit into the plot area.
  void RecalculatePlotBounds();

  // Description:
  // Process a rubber band selection event.
  virtual void ProcessSelectionEvent(vtkObject* caller, void* callData);

  // Description:
  // The X and Y axes for the chart
  vtkAxis *XAxis, *YAxis;

  // Description:
  // The grid for the chart.
  vtkPlotGrid *Grid;

  // Description:
  // The 2D transform for the series drawn in the plot area
  vtkTransform2D *PlotTransform;

  // Description:
  // Does the plot area transform need to be recalculated?
  bool PlotTransformValid;

  // Description:
  // The origin of the box when selecting a region of the chart.
  float BoxOrigin[2];

  // Description:
  // The width and height of the selection box.
  float BoxGeometry[2];

  // Description:
  // Should the box be drawn (could be selection, zoom etc.
  bool DrawBox;

  // Description:
  // Should we draw the location of the nearest point on the plot?
  bool DrawNearestPoint;

  // Description:
  // Keep the axes drawn at the origin? This will attempt to keep the axes drawn
  // at the origin, i.e. 0.0, 0.0 for the chart. This is often the preferred
  // way of drawing scientific/mathematical charts.
  bool DrawAxesAtOrigin;

private:
  vtkChartXY(const vtkChartXY &); // Not implemented.
  void operator=(const vtkChartXY &);   // Not implemented.

  vtkChartXYPrivate *ChartPrivate; // Private class where I hide my STL containers

  // Private functions to render different parts of the chart
  void RenderPlots(vtkContext2D *painter);

//ETX
};

#endif //__vtkChartXY_h
