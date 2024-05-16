// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkChartBox
 * @brief   Factory class for drawing box plot charts
 *
 *
 * This defines the interface for a box plot chart.
 */

#ifndef vtkChartBox_h
#define vtkChartBox_h

#include "vtkChart.h"
#include "vtkChartsCoreModule.h" // For export macro
#include "vtkWrappingHints.h"    // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkIdTypeArray;
class vtkPlotBox;
class vtkStdString;
class vtkStringArray;
class vtkTooltipItem;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkChartBox : public vtkChart
{
public:
  vtkTypeMacro(vtkChartBox, vtkChart);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a box chart
   */
  static vtkChartBox* New();

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

  ///@{
  /**
   * Set the visibility of the specified column.
   */
  void SetColumnVisibility(const vtkStdString& name, bool visible);
  void SetColumnVisibility(vtkIdType column, bool visible);
  ///@}

  /**
   * Set the visibility of all columns (true will make them all visible, false
   * will remove all visible columns).
   */
  void SetColumnVisibilityAll(bool visible);

  ///@{
  /**
   * Get the visibility of the specified column.
   */
  bool GetColumnVisibility(const vtkStdString& name);
  bool GetColumnVisibility(vtkIdType column);
  ///@}

  /**
   * Get the input table column id of a column by its name.
   */
  vtkIdType GetColumnId(const vtkStdString& name);

  ///@{
  /**
   * Get a list of the columns, and the order in which they are displayed.
   */
  vtkGetObjectMacro(VisibleColumns, vtkStringArray);
  ///@}

  // Index of the selected column in the visible columns list.
  vtkGetMacro(SelectedColumn, int);
  vtkSetMacro(SelectedColumn, int);

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  vtkPlot* GetPlot(vtkIdType index) override;

  /**
   * Get the number of plots the chart contains.
   */
  vtkIdType GetNumberOfPlots() override;

  /**
   * Get the chart Y axis
   */
  virtual vtkAxis* GetYAxis();

  /**
   * Get the column X position by index in the visible set.
   */
  virtual float GetXPosition(int index);

  /**
   * Get the number of visible box plots in the current chart.
   */
  virtual vtkIdType GetNumberOfVisibleColumns();

  /**
   * Set plot to use for the chart. Since this type of chart can
   * only contain one plot, this will replace the previous plot.
   */
  virtual void SetPlot(vtkPlotBox* plot);

  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button down event
   */
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button release event.
   */
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Set the vtkTooltipItem object that will be displayed by the chart.
   */
  virtual void SetTooltip(vtkTooltipItem* tooltip);

  /**
   * Get the vtkTooltipItem object that will be displayed by the chart.
   */
  virtual vtkTooltipItem* GetTooltip();

  /**
   * Set the information passed to the tooltip.
   */
  virtual void SetTooltipInfo(const vtkContextMouseEvent&, const vtkVector2d&, vtkIdType, vtkPlot*,
    vtkIdType segmentIndex = -1);

  /**
   * Calls superclass implementation and sets GeometryValid to False, causing
   * the chart's geometry to be updated on the next Paint call.
   */
  void SetSize(const vtkRectf& rect) override;

  /**
   * Calls superclass implementation and sets GeometryValid to False, causing
   * the chart's geometry to be updated on the next Paint call.
   */
  void SetGeometry(int arg1, int arg2) override;

  /**
   * Calls superclass implementation and sets GeometryValid to False, causing
   * the chart's geometry to be updated on the next Paint call.
   */
  void SetLayoutStrategy(int strategy) override;

protected:
  vtkChartBox();
  ~vtkChartBox() override;

  ///@{
  /**
   * Private storage object - where we hide all of our STL objects...
   */
  class Private;
  Private* Storage;
  ///@}

  bool GeometryValid;

  /**
   * Selected indices for the table the plot is rendering
   */
  vtkIdTypeArray* Selection;

  /**
   * A list of the visible columns in the chart.
   */
  vtkStringArray* VisibleColumns;

  ///@{
  /**
   * Index of the selected column in the visible columns list.
   */
  int SelectedColumn;
  float SelectedColumnDelta;
  ///@}

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  vtkTimeStamp BuildTime;

  /**
   * The tooltip item for the chart - can be used to display extra information.
   */
  vtkSmartPointer<vtkTooltipItem> Tooltip;

  void ResetSelection();
  void UpdateGeometry(vtkContext2D*);
  void CalculatePlotTransform();
  void SwapAxes(int a1, int a2);

  /**
   * Try to locate a point within the plots to display in a tooltip.
   * If invokeEvent is greater than 0, then an event will be invoked if a point
   * is at that mouse position.
   */
  bool LocatePointInPlots(const vtkContextMouseEvent& mouse, int invokeEvent = -1);

  int LocatePointInPlot(const vtkVector2f& position, const vtkVector2f& tolerance,
    vtkVector2f& plotPos, vtkPlot* plot, vtkIdType& segmentId);

private:
  vtkChartBox(const vtkChartBox&) = delete;
  void operator=(const vtkChartBox&) = delete;
};

///@{
/**
 * Small struct used by InvokeEvent to send some information about the point
 * that was clicked on. This is an experimental part of the API, subject to
 * change.
 */
struct vtkChartBoxData
{
  vtkStdString SeriesName;
  vtkVector2f Position;
  vtkVector2i ScreenPosition;
  int Index;
};
///@}

VTK_ABI_NAMESPACE_END
#endif // vtkChartBox_h
