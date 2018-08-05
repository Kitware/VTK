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

/**
 * @class   vtkChartXY
 * @brief   Factory class for drawing XY charts
 *
 *
 * This class implements an XY chart.
 *
 * @sa
 * vtkBarChartActor
*/

#ifndef vtkChartXY_h
#define vtkChartXY_h

#include "vtkChart.h"
#include "vtkChartsCoreModule.h" // For export macro
#include "vtkContextPolygon.h"   // For vtkContextPolygon
#include "vtkSmartPointer.h"     // For SP ivars
#include "vtkVector.h"           // For vtkVector2f in struct

class vtkPlot;
class vtkAxis;
class vtkPlotGrid;
class vtkChartLegend;
class vtkTooltipItem;
class vtkChartXYPrivate; // Private class to keep my STL vector in...

class VTKCHARTSCORE_EXPORT vtkChartXY : public vtkChart
{
public:
  vtkTypeMacro(vtkChartXY, vtkChart);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a 2D Chart object.
   */
  static vtkChartXY* New();

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
   * Add a plot to the chart, defaults to using the name of the y column
   */
  vtkPlot* AddPlot(int type) override;

  /**
   * Adds a plot to the chart
   */
  vtkIdType AddPlot(vtkPlot* plot) override;

  /**
   * Remove the plot at the specified index, returns true if successful,
   * false if the index was invalid.
   */
  bool RemovePlot(vtkIdType index) override;

  /**
   * Remove all plots from the chart.
   */
  void ClearPlots() override;

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  vtkPlot* GetPlot(vtkIdType index) override;

  /**
   * Get the index of the specified plot, returns -1 if the plot does not
   * belong to the chart.
   */
  virtual vtkIdType GetPlotIndex(vtkPlot*);

  /**
   * Raises the \a plot to the top of the plot's stack.
   * \return The new index of the plot
   * \sa StackPlotAbove(), LowerPlot(), StackPlotUnder()
   */
  vtkIdType RaisePlot(vtkPlot* plot);

  /**
   * Raises the \a plot above the \a under plot. If \a under is null,
   * the plot is raised to the top of the plot's stack.
   * \return The new index of the plot
   * \sa RaisePlot(), LowerPlot(), StackPlotUnder()
   */
  virtual vtkIdType StackPlotAbove(vtkPlot* plot, vtkPlot* under);

  /**
   * Lowers the \a plot to the bottom of the plot's stack.
   * \return The new index of the plot
   * \sa StackPlotUnder(), RaisePlot(), StackPlotAbove()
   */
  vtkIdType LowerPlot(vtkPlot* plot);

  /**
   * Lowers the \a plot under the \a above plot. If \a above is null,
   * the plot is lowered to the bottom of the plot's stack
   * \return The new index of the plot
   * \sa StackPlotUnder(), RaisePlot(), StackPlotAbove()
   */
  virtual vtkIdType StackPlotUnder(vtkPlot* plot, vtkPlot* above);

  /**
   * Get the number of plots the chart contains.
   */
  vtkIdType GetNumberOfPlots() override;

  /**
   * Figure out which quadrant the plot is in.
   */
  int GetPlotCorner(vtkPlot* plot);

  /**
   * Figure out which quadrant the plot is in.
   */
  void SetPlotCorner(vtkPlot* plot, int corner);

  /**
   * Get the axis specified by axisIndex. This is specified with the vtkAxis
   * position enum, valid values are vtkAxis::LEFT, vtkAxis::BOTTOM,
   * vtkAxis::RIGHT and vtkAxis::TOP.
   */
  vtkAxis* GetAxis(int axisIndex) override;

  /**
   * Set the axis specified by axisIndex. This is specified with the vtkAxis
   * position enum, valid values are vtkAxis::LEFT, vtkAxis::BOTTOM,
   * vtkAxis::RIGHT and vtkAxis::TOP.
   */
  virtual void SetAxis(int axisIndex, vtkAxis*) override;

  /**
   * Set whether the chart should draw a legend.
   */
  void SetShowLegend(bool visible) override;

  /**
   * Get the vtkChartLegend object that will be displayed by the chart.
   */
  vtkChartLegend* GetLegend() override;

  /**
   * Set the vtkTooltipItem object that will be displayed by the chart.
   */
  virtual void SetTooltip(vtkTooltipItem* tooltip);

  /**
   * Get the vtkTooltipItem object that will be displayed by the chart.
   */
  virtual vtkTooltipItem* GetTooltip();

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
   * Set the selection method, which controls how selections are handled by the
   * chart. The default is SELECTION_ROWS which selects all points in all plots
   * in a chart that have values in the rows selected. SELECTION_PLOTS allows
   * for finer-grained selections specific to each plot, and so to each XY
   * column pair.
   */
  void SetSelectionMethod(int method) override;

  /**
  * Remove all the selection from Plots
  */
  void RemovePlotSelections();

  //@{
  /**
   * If true then the axes will be drawn at the origin (scientific style).
   */
  vtkSetMacro(DrawAxesAtOrigin, bool);
  vtkGetMacro(DrawAxesAtOrigin, bool);
  vtkBooleanMacro(DrawAxesAtOrigin, bool);
  //@}

  //@{
  /**
   * If true then the axes will be turned on and off depending upon whether
   * any plots are in that corner. Defaults to true.
   */
  vtkSetMacro(AutoAxes, bool);
  vtkGetMacro(AutoAxes, bool);
  vtkBooleanMacro(AutoAxes, bool);
  //@}

  //@{
  /**
   * Border size of the axes that are hidden (vtkAxis::GetVisible())
   */
  vtkSetMacro(HiddenAxisBorder, int);
  vtkGetMacro(HiddenAxisBorder, int);
  //@}

  //@{
  /**
   * Force the axes to have their Minimum and Maximum properties inside the
   * plot boundaries. It constrains pan and zoom interaction.
   * False by default.
   */
  vtkSetMacro(ForceAxesToBounds, bool);
  vtkGetMacro(ForceAxesToBounds, bool);
  vtkBooleanMacro(ForceAxesToBounds, bool);
  //@}

  //@{
  /**
   * Set the width fraction for any bar charts drawn in this chart. It is
   * assumed that all bar plots will use the same array for the X axis, and that
   * this array is regularly spaced. The delta between the first two x values is
   * used to calculated the width of the bars, and subdivided between each bar.
   * The default value is 0.8, 1.0 would lead to bars that touch.
   */
  vtkSetMacro(BarWidthFraction, float);
  vtkGetMacro(BarWidthFraction, float);
  //@}

  //@{
  /**
   * Set the behavior of the mouse wheel.  If true, the mouse wheel zooms in/out
   * on the chart.  Otherwise, unless MouseWheelEvent is overridden by a subclass
   * the mouse wheel does nothing.
   * The default value is true.
   */
  vtkSetMacro(ZoomWithMouseWheel, bool);
  vtkGetMacro(ZoomWithMouseWheel, bool);
  vtkBooleanMacro(ZoomWithMouseWheel, bool);
  //@}

  //@{
  /**
   * Adjust the minimum of a logarithmic axis to be greater than 0, regardless
   * of the minimum data value.
   * False by default.
   */
  vtkSetMacro(AdjustLowerBoundForLogPlot, bool);
  vtkGetMacro(AdjustLowerBoundForLogPlot, bool);
  vtkBooleanMacro(AdjustLowerBoundForLogPlot, bool);
  //@}

  //@{
  /**
  * Set if the point can be dragged along X
  * by the ClickAndDrag Action
  * True by default.
  */
  vtkSetMacro(DragPointAlongX, bool);
  vtkGetMacro(DragPointAlongX, bool);
  vtkBooleanMacro(DragPointAlongX, bool);
  //@}

  //@{
  /**
  * Set if the point can be dragged along Y
  * by the ClickAndDrag Action
  * True by default.
  */
  vtkSetMacro(DragPointAlongY, bool);
  vtkGetMacro(DragPointAlongY, bool);
  vtkBooleanMacro(DragPointAlongY, bool);
  //@}

  /**
   * Set the information passed to the tooltip.
   */
  virtual void SetTooltipInfo(const vtkContextMouseEvent&, const vtkVector2d&, vtkIdType, vtkPlot*,
    vtkIdType segmentIndex = -1);

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
   * Key press event.
   */
  bool KeyPressEvent(const vtkContextKeyEvent& key) override;

protected:
  vtkChartXY();
  ~vtkChartXY() override;

  /**
   * Recalculate the necessary transforms.
   */
  void RecalculatePlotTransforms();

  /**
   * Calculate the optimal zoom level such that all of the points to be plotted
   * will fit into the plot area.
   */
  void RecalculatePlotBounds();

  /**
   * Update the layout of the chart, this may require the vtkContext2D in order
   * to get font metrics etc. Initially this was added to resize the charts
   * according in response to the size of the axes.
   */
  virtual bool UpdateLayout(vtkContext2D* painter);

  /**
   * Layout for the legend if it is visible. This is run after the axes layout
   * and will adjust the borders to account for the legend position.
   * \return The required space in the specified border.
   */
  virtual int GetLegendBorder(vtkContext2D* painter, int axisPosition);

  /**
   * Called after the edges of the chart are decided, set the position of the
   * legend, depends upon its alignment.
   */
  virtual void SetLegendPosition(const vtkRectf& rect);

  /**
   * The legend for the chart.
   */
  vtkSmartPointer<vtkChartLegend> Legend;

  /**
   * The tooltip item for the chart - can be used to display extra information.
   */
  vtkSmartPointer<vtkTooltipItem> Tooltip;

  /**
   * Does the plot area transform need to be recalculated?
   */
  bool PlotTransformValid;

  /**
   * The box created as the mouse is dragged around the screen.
   */
  vtkRectf MouseBox;

  /**
   * Should the box be drawn (could be selection, zoom etc).
   */
  bool DrawBox;

  /**
   * The polygon created as the mouse is dragged around the screen when in
   * polygonal selection mode.
   */
  vtkContextPolygon SelectionPolygon;

  /**
   * Should the selection polygon be drawn.
   */
  bool DrawSelectionPolygon;

  /**
   * Should we draw the location of the nearest point on the plot?
   */
  bool DrawNearestPoint;

  /**
   * Keep the axes drawn at the origin? This will attempt to keep the axes drawn
   * at the origin, i.e. 0.0, 0.0 for the chart. This is often the preferred
   * way of drawing scientific/mathematical charts.
   */
  bool DrawAxesAtOrigin;

  /**
   * Should axes be turned on and off automatically - defaults to on.
   */
  bool AutoAxes;

  /**
   * Size of the border when an axis is hidden
   */
  int HiddenAxisBorder;

  /**
   * The fraction of the interval taken up along the x axis by any bars that are
   * drawn on the chart.
   */
  float BarWidthFraction;

  /**
   * Property to force the axes to have their Minimum and Maximum properties
   * inside the plot boundaries. It constrains pan and zoom interaction.
   * False by default.
   */
  bool ForceAxesToBounds;

  /**
   * Property to enable zooming the chart with the mouse wheel.
   * True by default.
   */
  bool ZoomWithMouseWheel;

  /**
   * Property to adjust the minimum of a logarithmic axis to be greater than 0,
   * regardless of the minimum data value.
   */
  bool AdjustLowerBoundForLogPlot;

  /**
  * Properties to enable the drag of a point for the ClickAndDrag Action
  */
  bool DragPointAlongX;
  bool DragPointAlongY;

private:
  vtkChartXY(const vtkChartXY&) = delete;
  void operator=(const vtkChartXY&) = delete;

  vtkChartXYPrivate* ChartPrivate; // Private class where I hide my STL containers

  /**
   * Internal variable to handle update of drag:
   * true if a point has been selected by the user click.
   */
  bool DragPoint;

  /**
   * Figure out the spacing between the bar chart plots, and their offsets.
   */
  void CalculateBarPlots();

  /**
   * Try to locate a point within the plots to display in a tooltip.
   * If invokeEvent is greater than 0, then an event will be invoked if a point
   * is at that mouse position.
   */
  bool LocatePointInPlots(const vtkContextMouseEvent& mouse, int invokeEvent = -1);

  int LocatePointInPlot(const vtkVector2f& position, const vtkVector2f& tolerance,
    vtkVector2f& plotPos, vtkPlot* plot, vtkIdType& segmentIndex);

  /**
   * Remove the plot from the plot corners list.
   */
  bool RemovePlotFromCorners(vtkPlot* plot);

  void ZoomInAxes(vtkAxis* x, vtkAxis* y, float* orign, float* max);

  /**
  * Remove all the selection from Plots.
  * The method does not call InvokeEvent(vtkCommand::SelectionChangedEvent)
  */
  void ReleasePlotSelections();

  /**
   * Transform the selection box or polygon.
   */
  void TransformBoxOrPolygon(bool polygonMode, vtkTransform2D* transform,
    const vtkVector2f& mousePosition, vtkVector2f& min, vtkVector2f& max,
    vtkContextPolygon& polygon);
};

//@{
/**
 * Small struct used by InvokeEvent to send some information about the point
 * that was clicked on. This is an experimental part of the API, subject to
 * change.
 */
struct vtkChartPlotData
{
  vtkStdString SeriesName;
  vtkVector2f Position;
  vtkVector2i ScreenPosition;
  int Index;
};
//@}

#endif // vtkChartXY_h
