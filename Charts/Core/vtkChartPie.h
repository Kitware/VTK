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

/**
 * @class   vtkChartPie
 * @brief   Factory class for drawing pie charts
 *
 *
 * This class implements an pie chart.
*/

#ifndef vtkChartPie_h
#define vtkChartPie_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkChart.h"

class vtkChartLegend;
class vtkTooltipItem;
class vtkChartPiePrivate;

class VTKCHARTSCORE_EXPORT vtkChartPie : public vtkChart
{
public:
  vtkTypeMacro(vtkChartPie, vtkChart);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Creates a 2D Chart object.
   */
  static vtkChartPie *New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  virtual void Update();

  /**
   * Paint event for the chart, called whenever the chart needs to be drawn
   */
  virtual bool Paint(vtkContext2D *painter);

  /**
   * Add a plot to the chart.
   */
  virtual vtkPlot * AddPlot(int type);

  /**
   * Add a plot to the chart. Return the index of the plot, -1 if it failed.
   */
  virtual vtkIdType AddPlot(vtkPlot* plot)
    { return Superclass::AddPlot(plot); }

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  virtual vtkPlot* GetPlot(vtkIdType index);

  /**
   * Get the number of plots the chart contains.
   */
  virtual vtkIdType GetNumberOfPlots();

  /**
   * Set whether the chart should draw a legend.
   */
  virtual void SetShowLegend(bool visible);

  /**
   * Get the legend for the chart, if available. Can return NULL if there is no
   * legend.
   */
  virtual vtkChartLegend * GetLegend();

  /**
   * Set the vtkContextScene for the item, always set for an item in a scene.
   */
  virtual void SetScene(vtkContextScene *scene);

  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  /**
   * Mouse enter event.
   */
  virtual bool MouseEnterEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse move event.
   */
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse leave event.
   */
  virtual bool MouseLeaveEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse button down event
   */
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse button release event.
   */
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse wheel event, positive delta indicates forward movement of the wheel.
   */
  virtual bool MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta);

protected:
  vtkChartPie();
  ~vtkChartPie();

  /**
   * Recalculate the necessary transforms.
   */
  void RecalculatePlotTransforms();

  /**
   * The legend for the chart.
   */
  vtkChartLegend *Legend;

  /**
   * The tooltip item for the chart - can be used to display extra information.
   */
  vtkTooltipItem *Tooltip;

  /**
   * Does the plot area transform need to be recalculated?
   */
  bool PlotTransformValid;

private:
  vtkChartPie(const vtkChartPie &) VTK_DELETE_FUNCTION;
  void operator=(const vtkChartPie &) VTK_DELETE_FUNCTION;

  /**
   * Try to locate a point within the plots to display in a tooltip
   */
  bool LocatePointInPlots(const vtkContextMouseEvent &mouse);

  /**
   * Private implementation details
   */
  vtkChartPiePrivate *Private;

};

#endif //vtkChartPie_h
