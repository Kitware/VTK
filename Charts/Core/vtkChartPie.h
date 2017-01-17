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
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Creates a 2D Chart object.
   */
  static vtkChartPie *New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() VTK_OVERRIDE;

  /**
   * Paint event for the chart, called whenever the chart needs to be drawn
   */
  bool Paint(vtkContext2D *painter) VTK_OVERRIDE;

  /**
   * Add a plot to the chart.
   */
  vtkPlot * AddPlot(int type) VTK_OVERRIDE;

  /**
   * Add a plot to the chart. Return the index of the plot, -1 if it failed.
   */
  vtkIdType AddPlot(vtkPlot* plot) VTK_OVERRIDE
    { return Superclass::AddPlot(plot); }

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  vtkPlot* GetPlot(vtkIdType index) VTK_OVERRIDE;

  /**
   * Get the number of plots the chart contains.
   */
  vtkIdType GetNumberOfPlots() VTK_OVERRIDE;

  /**
   * Set whether the chart should draw a legend.
   */
  void SetShowLegend(bool visible) VTK_OVERRIDE;

  /**
   * Get the legend for the chart, if available. Can return NULL if there is no
   * legend.
   */
  vtkChartLegend * GetLegend() VTK_OVERRIDE;

  /**
   * Set the vtkContextScene for the item, always set for an item in a scene.
   */
  void SetScene(vtkContextScene *scene) VTK_OVERRIDE;

  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;

  /**
   * Mouse enter event.
   */
  bool MouseEnterEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;

  /**
   * Mouse move event.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;

  /**
   * Mouse leave event.
   */
  bool MouseLeaveEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;

  /**
   * Mouse button down event
   */
  bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;

  /**
   * Mouse button release event.
   */
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;

  /**
   * Mouse wheel event, positive delta indicates forward movement of the wheel.
   */
  bool MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta) VTK_OVERRIDE;

protected:
  vtkChartPie();
  ~vtkChartPie() VTK_OVERRIDE;

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
