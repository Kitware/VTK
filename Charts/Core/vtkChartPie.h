// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkChartPie
 * @brief   Factory class for drawing pie charts
 *
 *
 * This class implements an pie chart.
 */

#ifndef vtkChartPie_h
#define vtkChartPie_h

#include "vtkChart.h"
#include "vtkChartsCoreModule.h" // For export macro
#include "vtkWrappingHints.h"    // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkChartLegend;
class vtkTooltipItem;
class vtkChartPiePrivate;
class vtkPlotPie;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkChartPie : public vtkChart
{
public:
  vtkTypeMacro(vtkChartPie, vtkChart);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a 2D Chart object.
   */
  static vtkChartPie* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the chart, called whenever the chart needs to be drawn
   */
  bool Paint(vtkContext2D* painter) override;

  using vtkChart::AddPlot;
  /**
   * Add a plot to the chart.
   */
  vtkPlot* AddPlot(int type) override;

  /**
   * Set plot to use for the chart. Since this type of chart can
   * only contain one plot, this will replace the previous plot.
   */
  virtual void SetPlot(vtkPlotPie* plot);

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  vtkPlot* GetPlot(vtkIdType index) override;

  /**
   * Get the number of plots the chart contains.
   */
  vtkIdType GetNumberOfPlots() override;

  /**
   * Set whether the chart should draw a legend.
   */
  void SetShowLegend(bool visible) override;

  /**
   * Get the legend for the chart, if available. Can return nullptr if there is no
   * legend.
   */
  vtkChartLegend* GetLegend() override;

  /**
   * Set the vtkContextScene for the item, always set for an item in a scene.
   */
  void SetScene(vtkContextScene* scene) override;

  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse enter event.
   */
  bool MouseEnterEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse leave event.
   */
  bool MouseLeaveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button down event
   */
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button release event.
   */
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse wheel event, positive delta indicates forward movement of the wheel.
   */
  bool MouseWheelEvent(const vtkContextMouseEvent& mouse, int delta) override;

protected:
  vtkChartPie();
  ~vtkChartPie() override;

  /**
   * Recalculate the necessary transforms.
   */
  void RecalculatePlotTransforms();

  /**
   * The legend for the chart.
   */
  vtkChartLegend* Legend;

  /**
   * The tooltip item for the chart - can be used to display extra information.
   */
  vtkTooltipItem* Tooltip;

  /**
   * Does the plot area transform need to be recalculated?
   */
  bool PlotTransformValid;

private:
  vtkChartPie(const vtkChartPie&) = delete;
  void operator=(const vtkChartPie&) = delete;

  /**
   * Try to locate a point within the plots to display in a tooltip
   */
  bool LocatePointInPlots(const vtkContextMouseEvent& mouse);

  /**
   * Private implementation details
   */
  vtkChartPiePrivate* Private;
};

VTK_ABI_NAMESPACE_END
#endif // vtkChartPie_h
