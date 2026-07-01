// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkChartRadViz
 * @brief   Handle interaction for vtkPlotRadViz
 *
 */

#ifndef vtkChartRadViz_h
#define vtkChartRadViz_h

#include "vtkChart.h"
#include "vtkChartsCoreModule.h" // For export macro
#include "vtkWrappingHints.h"    // For VTK_MARSHALAUTO
#include <memory>

VTK_ABI_NAMESPACE_BEGIN
class vtkChartLegend;
class vtkTooltipItem;
class vtkPlotRadViz;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkChartRadViz : public vtkChart
{
public:
  vtkTypeMacro(vtkChartRadViz, vtkChart);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a RadViz chart object.
   */
  static vtkChartRadViz* New();

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
   * Add a plot to the chart. If a plot already exists it does nothing.
   * It returns the added or existing plot.
   */
  vtkPlot* AddPlot(int type) override;

  /**
   * Set plot to use for the chart. Since this type of chart can
   * only contain one plot, this will replace the previous plot.
   */
  virtual void SetPlot(vtkPlotRadViz* plot);

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  vtkPlot* GetPlot(vtkIdType index) override;

  /**
   * Get the number of plots the chart contains.
   */
  vtkIdType GetNumberOfPlots() override;

  /**
   * Set the vtkContextScene for the item, always set for an item in a scene.
   */
  void SetScene(vtkContextScene* scene) override;

  ///@{
  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse enter event.
   * Return true if the item holds the event, false if the event can be propagated to other items.
   */
  bool MouseEnterEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event.
   * Return true if the item holds the event, false if the event can be propagated to other items.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse leave event.
   * Return true if the item holds the event, false if the event can be propagated to other items.
   */
  bool MouseLeaveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button down event.
   * Return true if the item holds the event, false if the event can be propagated to other items.
   */
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button release event.
   * Return true if the item holds the event, false if the event can be propagated to other items.
   */
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse wheel event, positive delta indicates forward movement of the wheel.
   */
  bool MouseWheelEvent(const vtkContextMouseEvent& mouse, int delta) override;
  ///@}

protected:
  vtkChartRadViz();
  ~vtkChartRadViz() override;

private:
  vtkChartRadViz(const vtkChartRadViz&) = delete;
  void operator=(const vtkChartRadViz&) = delete;

  /**
   * Private implementation details
   */
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif // vtkChartRadViz_h
