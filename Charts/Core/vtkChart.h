/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChart.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkChart
 * @brief   Factory class for drawing 2D charts
 *
 *
 * This defines the interface for a chart.
*/

#ifndef vtkChart_h
#define vtkChart_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkContextItem.h"
#include "vtkRect.h"         // For vtkRectf
#include "vtkStdString.h"    // For vtkStdString ivars
#include "vtkSmartPointer.h" // For SP ivars

class vtkTransform2D;
class vtkContextScene;
class vtkPlot;
class vtkAxis;
class vtkBrush;
class vtkTextProperty;
class vtkChartLegend;

class vtkInteractorStyle;
class vtkAnnotationLink;

class VTKCHARTSCORE_EXPORT vtkChart : public vtkContextItem
{
public:
  vtkTypeMacro(vtkChart, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Enum of the available chart types
   */
  enum {
    LINE,
    POINTS,
    BAR,
    STACKED,
    BAG,
    FUNCTIONALBAG,
    AREA};

  /**
   * Enum of valid chart action types.

   * PAN - moves the axis range
   * ZOOM - zooms to a selected rectangle
   * ZOOM_AXIS - zooms the x and y axis range
   * SELECT_RECTANGLE - selects points within a rectangle
   * SELECT_POLYGON - selects points within a polygon
   * SELECT - alias for SELECT_RECTANGLE
   * NOTIFY - Post vtkCommand::InteractionEvent on selection of a point
   */
  enum {
    PAN = 0,
    ZOOM,
    ZOOM_AXIS,
    SELECT,
    SELECT_RECTANGLE = SELECT,
    SELECT_POLYGON,
    NOTIFY
  };

  /**
   * Enum of event type that are triggered by the charts
   */
  enum EventIds {
    UpdateRange = 1002
  };

  /**
   * Paint event for the chart, called whenever the chart needs to be drawn
   */
  virtual bool Paint(vtkContext2D *painter) = 0;

  /**
   * Add a plot to the chart, defaults to using the name of the y column
   */
  virtual vtkPlot* AddPlot(int type);

  /**
   * Add a plot to the chart. Return the index of the plot, -1 if it failed.
   */
  virtual vtkIdType AddPlot(vtkPlot* plot);

  /**
   * Remove the plot at the specified index, returns true if successful,
   * false if the index was invalid.
   */
  virtual bool RemovePlot(vtkIdType index);

  /**
   * Remove the given plot.  Returns true if successful, false if the plot
   * was not contained in this chart.  Note, the base implementation of
   * this method performs a linear search to locate the plot.
   */
  virtual bool RemovePlotInstance(vtkPlot* plot);

  /**
   * Remove all plots from the chart.
   */
  virtual void ClearPlots();

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  virtual vtkPlot* GetPlot(vtkIdType index);

  /**
   * Get the number of plots the chart contains.
   */
  virtual vtkIdType GetNumberOfPlots();

  /**
   * Get the axis specified by axisIndex. 0 is x, 1 is y. This should probably
   * be improved either using a string or enum to select the axis.
   */
  virtual vtkAxis* GetAxis(int axisIndex);

  /**
   * Get the number of axes in the current chart.
   */
  virtual vtkIdType GetNumberOfAxes();

  /**
   * Request that the chart recalculates the range of its axes. Especially
   * useful in applications after the parameters of plots have been modified.
   */
  virtual void RecalculateBounds();

  /**
   * Enumeration of the possible selection methods in a chart. SELECTION_ROWS
   * is the default and simply selects the row in a table in all plots showing
   * that table. SELECTION_PLOTS will make a selection in each plot, and that
   * selection remains specific to the plot object. SELECTION_COLUMNS selects
   * the plots that use as input the selected columns of a table.
   */
  enum {
    SELECTION_ROWS,
    SELECTION_PLOTS,
    SELECTION_COLUMNS
  };

  //@{
  /**
   * Set the selection method, which controls how selections are handled by the
   * chart. The default is SELECTION_ROWS which selects all points in all plots
   * in a chart that have values in the rows selected. SELECTION_PLOTS allows
   * for finer-grained selections specific to each plot, and so to each XY
   * column pair. SELECTION_COLUMNS selects all points of plots that
   * correspond to selected columns.
   */
  virtual void SetSelectionMethod(int method);
  virtual int GetSelectionMethod();
  //@}

  /**
   * Set the vtkAnnotationLink for the chart.
   */
  virtual void SetAnnotationLink(vtkAnnotationLink *link);

  //@{
  /**
   * Get the vtkAnnotationLink for the chart.
   */
  vtkGetObjectMacro(AnnotationLink, vtkAnnotationLink);
  //@}

  //@{
  /**
   * Set/get the width and the height of the chart.
   */
  vtkSetVector2Macro(Geometry, int);
  vtkGetVector2Macro(Geometry, int);
  //@}

  //@{
  /**
   * Set/get the first point in the chart (the bottom left).
   */
  vtkSetVector2Macro(Point1, int);
  vtkGetVector2Macro(Point1, int);
  //@}

  //@{
  /**
   * Set/get the second point in the chart (the top right).
   */
  vtkSetVector2Macro(Point2, int);
  vtkGetVector2Macro(Point2, int);
  //@}

  //@{
  /**
   * Set/get whether the chart should draw a legend.
   */
  virtual void SetShowLegend(bool visible);
  virtual bool GetShowLegend();
  //@}

  /**
   * Get the legend for the chart, if available. Can return NULL if there is no
   * legend.
   */
  virtual vtkChartLegend * GetLegend();

  //@{
  /**
   * Get/set the title text of the chart.
   */
  virtual void SetTitle(const vtkStdString &title);
  virtual vtkStdString GetTitle();
  //@}

  //@{
  /**
   * Get the vtkTextProperty that governs how the chart title is displayed.
   */
  vtkGetObjectMacro(TitleProperties, vtkTextProperty);
  //@}

  //@{
  /**
   * Set/get the borders of the chart (space in pixels around the chart).
   */
  void SetBottomBorder(int border);
  void SetTopBorder(int border);
  void SetLeftBorder(int border);
  void SetRightBorder(int border);
  //@}

  /**
   * Set/get the borders of the chart (space in pixels around the chart).
   */
  void SetBorders(int left, int bottom, int right, int top);

  /**
   * Set the size of the chart. The rect argument specifies the bottom corner,
   * width and height of the chart. The borders will be laid out within the
   * specified rectangle.
   */
  void SetSize(const vtkRectf &rect);

  /**
   * Get the current size of the chart.
   */
  vtkRectf GetSize();

  /**
   * Enum of the available layout strategies for the charts.
   */
  enum {
    FILL_SCENE,  // Attempt to fill the entire scene.
    FILL_RECT,   // Attempt to supply the supplied vtkRectf in Size.
    AXES_TO_RECT // Put the corners of the axes on the vtkRectf in Size.
  };

  //@{
  /**
   * Set/get the layout strategy that should be used by the chart. As we don't
   * support enums this can take any value in the integer range, but the only
   * valid enums are FILL_SCENE, FILL_RECT and AXES_TO_RECT.
   */
  vtkSetMacro(LayoutStrategy, int);
  vtkGetMacro(LayoutStrategy, int);
  //@}

  //@{
  /**
   * Set/get whether the chart should automatically resize to fill the current
   * render window. Default is true.
   */
  virtual void SetAutoSize(bool isAutoSized)
  {
    this->LayoutStrategy = isAutoSized ? vtkChart::FILL_SCENE :
                                         vtkChart::FILL_RECT;
  }
  virtual bool GetAutoSize()
  {
    return this->LayoutStrategy == vtkChart::FILL_SCENE ? true : false;
  }
  //@}

  //@{
  /**
   * Set/get whether the chart should still render its axes and decorations
   * even if the chart has no visible plots. Default is false (do not render
   * an empty plot).

   * Note that if you wish to render axes for an empty plot you should also
   * set AutoSize to false, as that will hide all axes for an empty plot.
   */
  vtkSetMacro(RenderEmpty, bool);
  vtkGetMacro(RenderEmpty, bool);
  //@}

  /**
   * Assign action types to mouse buttons. Available action types are PAN, ZOOM
   * and SELECT in the chart enum, the default assigns the LEFT_BUTTON to
   * PAN, MIDDLE_BUTTON to ZOOM and RIGHT_BUTTON to SELECT. Valid mouse enums
   * are in the vtkContextMouseEvent class.

   * Note that only one mouse button can be assigned to each action, an action
   * will have -1 (invalid button) assigned if it had the same button as the one
   * assigned to a different action.
   */
  virtual void SetActionToButton(int action, int button);

  /**
   * Get the mouse button associated with the supplied action. The mouse button
   * enum is from vtkContextMouseEvent, and the action enum is from vtkChart.
   */
  virtual int GetActionToButton(int action);

  /**
   * Assign action types to single mouse clicks. Available action types are
   * SELECT and NOTIFY in the chart enum. The default assigns the LEFT_BUTTON
   * to NOTIFY, and the RIGHT_BUTTON to SELECT.
   */
  virtual void SetClickActionToButton(int action, int button);

  /**
   * Get the mouse button associated with the supplied click action. The mouse
   * button enum is from vtkContextMouseEvent, and the action enum is from
   * vtkChart.
   */
  virtual int GetClickActionToButton(int action);

  //@{
  /**
   * Set/Get the brush to use for the background color.
   */
  void SetBackgroundBrush(vtkBrush *brush);
  vtkBrush* GetBackgroundBrush();
  //@}

  //@{
  /**
   * Set/get the Selection Mode that will be used by the chart while doing
   * selection. The only valid enums are vtkContextScene::SELECTION_NONE,
   * SELECTION_DEFAULT, SELECTION_ADDITION, SELECTION_SUBTRACTION, SELECTION_TOGGLE
   */
  virtual void SetSelectionMode(int);
  vtkGetMacro(SelectionMode, int);
  //@}

protected:
  vtkChart();
  ~vtkChart();

  /**
   * Given the x and y vtkAxis, and a transform, calculate the transform that
   * the points in a chart would need to be drawn within the axes. This assumes
   * that the axes have the correct start and end positions, and that they are
   * perpendicular.
   */
  bool CalculatePlotTransform(vtkAxis *x, vtkAxis *y,
                              vtkTransform2D *transform);

  /**
   * Calculate the unshifted, and unscaled plot transform for the x and y axis.
   */
  bool CalculateUnscaledPlotTransform(vtkAxis *x, vtkAxis *y,
                                      vtkTransform2D *transform);

  /**
   * Attach axis range listener so we can forward those events at the chart level
   */
  void AttachAxisRangeListener(vtkAxis*);

  void AxisRangeForwarderCallback(vtkObject*,unsigned long, void*);

  /**
   * Our annotation link, used for sharing selections etc.
   */
  vtkAnnotationLink *AnnotationLink;

  /**
   * The width and the height of the chart.
   */
  int Geometry[2];

  /**
   * The position of the lower left corner of the chart.
   */
  int Point1[2];

  /**
   * The position of the upper right corner of the chart.
   */
  int Point2[2];

  /**
   * Display the legend?
   */
  bool ShowLegend;

  /**
   * The title of the chart
   */
  vtkStdString Title;

  /**
   * The text properties associated with the chart
   */
  vtkTextProperty* TitleProperties;

  vtkRectf Size;
  // The layout strategy to employ when fitting the chart into the space.
  int LayoutStrategy;
  bool RenderEmpty;

  /**
   * Brush to use for drawing the background.
   */
  vtkSmartPointer<vtkBrush> BackgroundBrush;

  // The mode when the chart is doing selection.
  int SelectionMode;

  // How plot selections are handled, SELECTION_ROWS (default) or
  // SELECTION_PLOTS - based on the plot that created the selection.
  int SelectionMethod;

  //@{
  /**
   * Hold mouse action mappings.
   */
  class MouseActions
  {
  public:
    MouseActions();
    enum { MaxAction = 5 };
    short& Pan() { return Data[0]; }
    short& Zoom() { return Data[1]; }
    short& ZoomAxis() { return Data[2]; }
    short& Select() { return Data[3]; }
    short& SelectPolygon() { return Data[4]; }
    short& operator[](int index) { return Data[index]; }
    short Data[MaxAction];
  };
  class MouseClickActions
  {
  public:
    MouseClickActions();
    short& Notify() { return Data[0]; }
    short& Select() { return Data[1]; }
    short& operator[](int index) { return Data[index]; }
    short Data[2];
  };
  //@}

  MouseActions Actions;
  MouseClickActions ActionsClick;

private:
  vtkChart(const vtkChart &) VTK_DELETE_FUNCTION;
  void operator=(const vtkChart &) VTK_DELETE_FUNCTION;
};

#endif //vtkChart_h
