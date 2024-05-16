// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkChartParallelCoordinates
 * @brief   Factory class for drawing 2D charts
 *
 *
 * This defines the interface for a parallel coordinates chart.
 */

#ifndef vtkChartParallelCoordinates_h
#define vtkChartParallelCoordinates_h

#include "vtkChart.h"
#include "vtkChartsCoreModule.h" // For export macro
#include "vtkNew.h"              // For vtkNew
#include "vtkWrappingHints.h"    // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkIdTypeArray;
class vtkStdString;
class vtkStringArray;
class vtkPlotParallelCoordinates;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkChartParallelCoordinates : public vtkChart
{
public:
  vtkTypeMacro(vtkChartParallelCoordinates, vtkChart);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a parallel coordinates chart
   */
  static vtkChartParallelCoordinates* New();

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

  /**
   * Draw a rect on a specific axis
   */
  bool PaintRect(vtkContext2D* painter, int axis, float min, float max);

  /**
   * Set the visibility of the specified column.
   */
  void SetColumnVisibility(const vtkStdString& name, bool visible);

  /**
   * Set the visibility of all columns (true will make them all visible, false
   * will remove all visible columns).
   */
  void SetColumnVisibilityAll(bool visible);

  /**
   * Get the visibility of the specified column.
   */
  bool GetColumnVisibility(const vtkStdString& name);

  /**
   * Get a list of the columns, and the order in which they are displayed.
   */
  virtual vtkStringArray* GetVisibleColumns();

  /**
   * Set the list of visible columns, and the order in which they will be displayed.
   */
  virtual void SetVisibleColumns(vtkStringArray* visColumns);

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
   * Get the axis specified by axisIndex.
   */
  vtkAxis* GetAxis(int axisIndex) override;

  /**
   * Get the number of axes in the current chart.
   */
  vtkIdType GetNumberOfAxes() override;

  /**
   * Request that the chart recalculates the range of its axes. Especially
   * useful in applications after the parameters of plots have been modified.
   */
  void RecalculateBounds() override;

  /**
   * Set plot to use for the chart. Since this type of chart can
   * only contain one plot, this will replace the previous plot.
   */
  virtual void SetPlot(vtkPlotParallelCoordinates* plot);

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

  /**
   * Update the selection of an axis based on the current selectionMode we
   * have previously set.
   */
  void UpdateCurrentAxisSelection(int axisId);

protected:
  vtkChartParallelCoordinates();
  ~vtkChartParallelCoordinates() override;

  ///@{
  /**
   * Private storage object - where we hide all of our STL objects...
   */
  struct Private;
  Private* Storage;
  ///@}

  /**
   * The legend for the chart.
   */
  vtkChartLegend* Legend;

  bool GeometryValid;

  /**
   * Selected indices for the table the plot is rendering
   */
  vtkIdTypeArray* Selection;

  /**
   * Strongly owned internal data for the column visibility.
   */
  vtkNew<vtkStringArray> VisibleColumns;

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  vtkTimeStamp BuildTime;

  void ResetSelection();
  void ResetAxeSelection(int axe);
  void ResetAxesSelection();
  void UpdateGeometry(vtkContext2D* painter);
  void CalculatePlotTransform();
  void SwapAxes(int a1, int a2);

private:
  vtkChartParallelCoordinates(const vtkChartParallelCoordinates&) = delete;
  void operator=(const vtkChartParallelCoordinates&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkChartParallelCoordinates_h
