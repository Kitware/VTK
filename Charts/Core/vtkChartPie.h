/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartPie.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkChartPie - Factory class for drawing pie charts
//
// .SECTION Description
// This class implements an pie chart.

#ifndef __vtkChartPie_h
#define __vtkChartPie_h

#include "vtkChart.h"

class vtkChartLegend;
class vtkTooltipItem;
class vtkChartPiePrivate;

class VTK_CHARTS_EXPORT vtkChartPie : public vtkChart
{
public:
  vtkTypeMacro(vtkChartPie, vtkChart);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Chart object.
  static vtkChartPie *New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Add a plot to the chart.
  virtual vtkPlot * AddPlot(int type);

  // Description:
  // Add a plot to the chart. Return the index of the plot, -1 if it failed.
  virtual vtkIdType AddPlot(vtkPlot* plot)
    { return Superclass::AddPlot(plot); }

  // Description:
  // Get the plot at the specified index, returns null if the index is invalid.
  virtual vtkPlot* GetPlot(vtkIdType index);

  // Description:
  // Get the number of plots the chart contains.
  virtual vtkIdType GetNumberOfPlots();

  // Description:
  // Set whether the chart should draw a legend.
  virtual void SetShowLegend(bool visible);

  // Description:
  // Get the legend for the chart, if available. Can return NULL if there is no
  // legend.
  virtual vtkChartLegend * GetLegend();

  // Description:
  // Set the vtkContextScene for the item, always set for an item in a scene.
  virtual void SetScene(vtkContextScene *scene);

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
  vtkChartPie();
  ~vtkChartPie();

  // Description:
  // Recalculate the necessary transforms.
  void RecalculatePlotTransforms();

  // Description:
  // The legend for the chart.
  vtkChartLegend *Legend;

  // Description:
  // The tooltip item for the chart - can be used to display extra information.
  vtkTooltipItem *Tooltip;

  // Description:
  // Does the plot area transform need to be recalculated?
  bool PlotTransformValid;

private:
  vtkChartPie(const vtkChartPie &); // Not implemented.
  void operator=(const vtkChartPie &);   // Not implemented.

  // Description:
  // Try to locate a point within the plots to display in a tooltip
  bool LocatePointInPlots(const vtkContextMouseEvent &mouse);

  // Description:
  // Private implementation details
  vtkChartPiePrivate *Private;

//ETX
};

#endif //__vtkChartPie_h
