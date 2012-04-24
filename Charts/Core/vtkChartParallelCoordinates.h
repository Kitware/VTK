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
// This defines the interface for a parallel coordinates chart.

#ifndef __vtkChartParallelCoordinates_h
#define __vtkChartParallelCoordinates_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkChart.h"

class vtkIdTypeArray;
class vtkStdString;
class vtkStringArray;
class vtkPlotParallelCoordinates;

class VTKCHARTSCORE_EXPORT vtkChartParallelCoordinates : public vtkChart
{
public:
  vtkTypeMacro(vtkChartParallelCoordinates, vtkChart);
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
  // Set the visibility of the specified column.
  void SetColumnVisibility(const vtkStdString& name, bool visible);

  // Description:
  // Set the visibility of all columns (true will make them all visible, false
  // will remove all visible columns).
  void SetColumnVisibilityAll(bool visible);

  // Description:
  // Get the visibility of the specified column.
  bool GetColumnVisibility(const vtkStdString& name);

  // Description:
  // Get a list of the columns, and the order in which they are displayed.
  vtkGetObjectMacro(VisibleColumns, vtkStringArray);

  // Description:
  // Get the plot at the specified index, returns null if the index is invalid.
  virtual vtkPlot* GetPlot(vtkIdType index);

  // Description:
  // Get the number of plots the chart contains.
  virtual vtkIdType GetNumberOfPlots();

  // Description:
  // Get the axis specified by axisIndex.
  virtual vtkAxis* GetAxis(int axisIndex);

  // Description:
  // Get the number of axes in the current chart.
  virtual vtkIdType GetNumberOfAxes();

  // Description:
  // Request that the chart recalculates the range of its axes. Especially
  // useful in applications after the parameters of plots have been modified.
  virtual void RecalculateBounds();

  // Description
  // Set plot to use for the chart. Since this type of chart can
  // only contain one plot, this will replace the previous plot.
  virtual void SetPlot(vtkPlotParallelCoordinates *plot);

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

  // Description:
  // A list of the visible columns in the chart.
  vtkStringArray *VisibleColumns;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

  void ResetSelection();
  void UpdateGeometry();
  void CalculatePlotTransform();
  void SwapAxes(int a1, int a2);

private:
  vtkChartParallelCoordinates(const vtkChartParallelCoordinates &); // Not implemented.
  void operator=(const vtkChartParallelCoordinates &);   // Not implemented.
//ETX
};

#endif //__vtkChartParallelCoordinates_h
