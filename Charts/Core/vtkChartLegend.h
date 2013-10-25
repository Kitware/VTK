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

// .NAME vtkChartLegend - draw the chart legend
//
// .SECTION Description
// The vtkChartLegend is drawn in screen coordinates. It is usually one of the
// last elements of a chart to be drawn. It renders the the mark/line for each
// plot, and the plot labels.

#ifndef __vtkChartLegend_h
#define __vtkChartLegend_h

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

  // Description:
  // Creates a 2D Chart object.
  static vtkChartLegend *New();

  // Description:
  // Set point the legend box is anchored to.
  vtkSetVector2Macro(Point, float);

  // Description:
  // Get point the legend box is anchored to.
  vtkGetVector2Macro(Point, float);

  enum {
    LEFT = 0,
    CENTER,
    RIGHT,
    TOP,
    BOTTOM,
    CUSTOM
    };

  // Description:
  // Set point the legend box is anchored to.
  void SetPoint(const vtkVector2f &point);

  // Description:
  // Get point the legend box is anchored to.
  const vtkVector2f& GetPointVector();

  // Description:
  // Set the horizontal alignment of the legend to the point specified.
  // Valid values are LEFT, CENTER and RIGHT.
  vtkSetMacro(HorizontalAlignment, int);

  // Description:
  // Get the horizontal alignment of the legend to the point specified.
  vtkGetMacro(HorizontalAlignment, int);

  // Description:
  // Set the vertical alignment of the legend to the point specified.
  // Valid values are TOP, CENTER and BOTTOM.
  vtkSetMacro(VerticalAlignment, int);

  // Description:
  // Get the vertical alignment of the legend to the point specified.
  vtkGetMacro(VerticalAlignment, int);

  // Description:
  // Set the padding between legend marks, default is 5.
  vtkSetMacro(Padding, int);

  // Description:
  // Get the padding between legend marks.
  vtkGetMacro(Padding, int);

  // Description:
  // Set the symbol width, default is 15.
  vtkSetMacro(SymbolWidth, int);

  // Description:
  // Get the legend symbol width.
  vtkGetMacro(SymbolWidth, int);

  // Description:
  // Set the point size of the label text.
  virtual void SetLabelSize(int size);

  // Description:
  // Get the point size of the label text.
  virtual int GetLabelSize();

  // Description:
  // Get/set if the legend should be drawn inline (inside the chart), or not.
  // True would generally request that the chart draws it inside the chart,
  // false would adjust the chart axes and make space to draw the axes outside.
  vtkSetMacro(Inline, bool);
  vtkGetMacro(Inline, bool);

  // Description:
  // Get/set if the legend can be dragged with the mouse button, or not.
  // True results in left click and drag causing the legend to move around the
  // scene. False disables response to mouse events.
  // The default is true.
  vtkSetMacro(DragEnabled, bool);
  vtkGetMacro(DragEnabled, bool);

  // Description:
  // Set the chart that the legend belongs to and will draw the legend for.
  void SetChart(vtkChart* chart);

  // Description:
  // Get the chart that the legend belongs to and will draw the legend for.
  vtkChart* GetChart();

  // Description:
  // Update the geometry of the axis. Takes care of setting up the tick mark
  // locations etc. Should be called by the scene before rendering.
  virtual void Update();

  // Description:
  // Paint event for the axis, called whenever the axis needs to be drawn.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Request the space the legend requires to be drawn. This is returned as a
  // vtkRect4f, with the corner being the offset from Point, and the width/
  // height being the total width/height required by the axis. In order to
  // ensure the numbers are correct, Update() should be called first.
  virtual vtkRectf GetBoundingRect(vtkContext2D* painter);

  // Description:
  // Get the pen used to draw the legend outline.
  vtkPen * GetPen();

  // Description:
  // Get the brush used to draw the legend background.
  vtkBrush * GetBrush();

  // Description:
  // Get the vtkTextProperty for the legend's labels.
  vtkTextProperty * GetLabelProperties();

  // Description:
  // Toggle whether or not this legend should attempt to cache its position
  // and size.  The default value is true.  If this value is set to false,
  // the legend will recalculate its position and bounds every time it is
  // drawn.  If users will be able to zoom in or out on your legend, you
  // may want to set this to false.  Otherwise, the border around the legend
  // may not resize appropriately.
  vtkSetMacro(CacheBounds, bool);
  vtkGetMacro(CacheBounds, bool);
  vtkBooleanMacro(CacheBounds, bool);

  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button down event
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button release event.
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

protected:
  vtkChartLegend();
  ~vtkChartLegend();

  float* Point;  // The point the legend is anchored to.
  int HorizontalAlignment; // Alignment of the legend to the point it is anchored to.
  int VerticalAlignment; // Alignment of the legend to the point it is anchored to.

  // Description:
  // The pen used to draw the legend box.
  vtkNew<vtkPen> Pen;

  // Description:
  // The brush used to render the background of the legend.
  vtkNew<vtkBrush> Brush;

  // Description:
  // The text properties of the labels used in the legend.
  vtkNew<vtkTextProperty> LabelProperties;

  // Description:
  // Should we move the legend box around in response to the mouse drag?
  bool DragEnabled;

  // Description:
  // Should the legend attempt to avoid recalculating its position &
  // bounds unnecessarily?
  bool CacheBounds;

  // Description:
  // Last button to be pressed.
  int Button;

  vtkTimeStamp PlotTime;
  vtkTimeStamp RectTime;

  vtkRectf Rect;

  // Description:
  // Padding between symbol and text.
  int Padding;

  // Description:
  // Width of the symbols in pixels in the legend.
  int SymbolWidth;

  // Description:
  // Should the legend be drawn inline in its chart?
  bool Inline;

  // Private storage class
  class Private;
  Private* Storage;

private:
  vtkChartLegend(const vtkChartLegend &); // Not implemented.
  void operator=(const vtkChartLegend &); // Not implemented.
};

#endif //__vtkChartLegend_h
