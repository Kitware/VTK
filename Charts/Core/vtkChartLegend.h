/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartLegend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkChartLegend
 * @brief   draw the chart legend
 *
 *
 * The vtkChartLegend is drawn in screen coordinates. It is usually one of the
 * last elements of a chart to be drawn. It renders the the mark/line for each
 * plot, and the plot labels.
*/

#ifndef vtkChartLegend_h
#define vtkChartLegend_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkContextItem.h"
#include "vtkNew.h"         // For vtkNew
#include "vtkRect.h"        // For vtkRectf return value

class vtkChart;
class vtkPen;
class vtkBrush;
class vtkTextProperty;

class VTKCHARTSCORE_EXPORT vtkChartLegend : public vtkContextItem
{
public:
  vtkTypeMacro(vtkChartLegend, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Creates a 2D Chart object.
   */
  static vtkChartLegend *New();

  //@{
  /**
   * Set point the legend box is anchored to.
   */
  vtkSetVector2Macro(Point, float);
  //@}

  //@{
  /**
   * Get point the legend box is anchored to.
   */
  vtkGetVector2Macro(Point, float);
  //@}

  enum {
    LEFT = 0,
    CENTER,
    RIGHT,
    TOP,
    BOTTOM,
    CUSTOM
  };

  /**
   * Set point the legend box is anchored to.
   */
  void SetPoint(const vtkVector2f &point);

  /**
   * Get point the legend box is anchored to.
   */
  const vtkVector2f& GetPointVector();

  //@{
  /**
   * Set the horizontal alignment of the legend to the point specified.
   * Valid values are LEFT, CENTER and RIGHT.
   */
  vtkSetMacro(HorizontalAlignment, int);
  //@}

  //@{
  /**
   * Get the horizontal alignment of the legend to the point specified.
   */
  vtkGetMacro(HorizontalAlignment, int);
  //@}

  //@{
  /**
   * Set the vertical alignment of the legend to the point specified.
   * Valid values are TOP, CENTER and BOTTOM.
   */
  vtkSetMacro(VerticalAlignment, int);
  //@}

  //@{
  /**
   * Get the vertical alignment of the legend to the point specified.
   */
  vtkGetMacro(VerticalAlignment, int);
  //@}

  //@{
  /**
   * Set the padding between legend marks, default is 5.
   */
  vtkSetMacro(Padding, int);
  //@}

  //@{
  /**
   * Get the padding between legend marks.
   */
  vtkGetMacro(Padding, int);
  //@}

  //@{
  /**
   * Set the symbol width, default is 15.
   */
  vtkSetMacro(SymbolWidth, int);
  //@}

  //@{
  /**
   * Get the legend symbol width.
   */
  vtkGetMacro(SymbolWidth, int);
  //@}

  /**
   * Set the point size of the label text.
   */
  virtual void SetLabelSize(int size);

  /**
   * Get the point size of the label text.
   */
  virtual int GetLabelSize();

  //@{
  /**
   * Get/set if the legend should be drawn inline (inside the chart), or not.
   * True would generally request that the chart draws it inside the chart,
   * false would adjust the chart axes and make space to draw the axes outside.
   */
  vtkSetMacro(Inline, bool);
  vtkGetMacro(Inline, bool);
  //@}

  //@{
  /**
   * Get/set if the legend can be dragged with the mouse button, or not.
   * True results in left click and drag causing the legend to move around the
   * scene. False disables response to mouse events.
   * The default is true.
   */
  vtkSetMacro(DragEnabled, bool);
  vtkGetMacro(DragEnabled, bool);
  //@}

  /**
   * Set the chart that the legend belongs to and will draw the legend for.
   */
  void SetChart(vtkChart* chart);

  /**
   * Get the chart that the legend belongs to and will draw the legend for.
   */
  vtkChart* GetChart();

  /**
   * Update the geometry of the axis. Takes care of setting up the tick mark
   * locations etc. Should be called by the scene before rendering.
   */
  virtual void Update();

  /**
   * Paint event for the axis, called whenever the axis needs to be drawn.
   */
  virtual bool Paint(vtkContext2D *painter);

  /**
   * Request the space the legend requires to be drawn. This is returned as a
   * vtkRect4f, with the corner being the offset from Point, and the width/
   * height being the total width/height required by the axis. In order to
   * ensure the numbers are correct, Update() should be called first.
   */
  virtual vtkRectf GetBoundingRect(vtkContext2D* painter);

  /**
   * Get the pen used to draw the legend outline.
   */
  vtkPen * GetPen();

  /**
   * Get the brush used to draw the legend background.
   */
  vtkBrush * GetBrush();

  /**
   * Get the vtkTextProperty for the legend's labels.
   */
  vtkTextProperty * GetLabelProperties();

  //@{
  /**
   * Toggle whether or not this legend should attempt to cache its position
   * and size.  The default value is true.  If this value is set to false,
   * the legend will recalculate its position and bounds every time it is
   * drawn.  If users will be able to zoom in or out on your legend, you
   * may want to set this to false.  Otherwise, the border around the legend
   * may not resize appropriately.
   */
  vtkSetMacro(CacheBounds, bool);
  vtkGetMacro(CacheBounds, bool);
  vtkBooleanMacro(CacheBounds, bool);
  //@}

  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  /**
   * Mouse move event.
   */
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse button down event
   */
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse button release event.
   */
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

protected:
  vtkChartLegend();
  ~vtkChartLegend();

  float* Point;  // The point the legend is anchored to.
  int HorizontalAlignment; // Alignment of the legend to the point it is anchored to.
  int VerticalAlignment; // Alignment of the legend to the point it is anchored to.

  /**
   * The pen used to draw the legend box.
   */
  vtkNew<vtkPen> Pen;

  /**
   * The brush used to render the background of the legend.
   */
  vtkNew<vtkBrush> Brush;

  /**
   * The text properties of the labels used in the legend.
   */
  vtkNew<vtkTextProperty> LabelProperties;

  /**
   * Should we move the legend box around in response to the mouse drag?
   */
  bool DragEnabled;

  /**
   * Should the legend attempt to avoid recalculating its position &
   * bounds unnecessarily?
   */
  bool CacheBounds;

  /**
   * Last button to be pressed.
   */
  int Button;

  vtkTimeStamp PlotTime;
  vtkTimeStamp RectTime;

  vtkRectf Rect;

  /**
   * Padding between symbol and text.
   */
  int Padding;

  /**
   * Width of the symbols in pixels in the legend.
   */
  int SymbolWidth;

  /**
   * Should the legend be drawn inline in its chart?
   */
  bool Inline;

  // Private storage class
  class Private;
  Private* Storage;

private:
  vtkChartLegend(const vtkChartLegend &) VTK_DELETE_FUNCTION;
  void operator=(const vtkChartLegend &) VTK_DELETE_FUNCTION;
};

#endif //vtkChartLegend_h
