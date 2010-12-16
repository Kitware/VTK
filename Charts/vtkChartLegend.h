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

#include "vtkContextItem.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer
#include "vtkVector.h"       // For vtkRectf return value

class vtkChart;
class vtkPen;
class vtkBrush;
class vtkTextProperty;

class VTK_CHARTS_EXPORT vtkChartLegend : public vtkContextItem
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
    BOTTOM
    };

  // Description:
  // Set point the legend box is anchored to.
  void SetPoint(const vtkVector2f &point);

  // Description:
  // Get point the legend box is anchored to.
  const vtkVector2f& GetPointVector();

  // Description:
  // Set the horizontal alignment of the legend to the point specified.
  vtkSetMacro(HorizontalAlignment, int);

  // Description:
  // Get the horizontal alignment of the legend to the point specified.
  vtkGetMacro(HorizontalAlignment, int);

  // Description:
  // Set the vertical alignment of the legend to the point specified.
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
  // Get the vtkTextProperty for the legend's labels.
  vtkTextProperty * GetLabelProperties();

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
  vtkRectf GetBoundingRect(vtkContext2D* painter);

protected:
  vtkChartLegend();
  ~vtkChartLegend();

  float* Point;  // The point the legend is anchored to.
  int HorizontalAlignment; // Alignment of the legend to the point it is anchored to.
  int VerticalAlignment; // Alignment of the legend to the point it is anchored to.

  // Description:
  // The pen used to draw the legend box.
  vtkSmartPointer<vtkPen> Pen;

  // Description:
  // The brush used to render the background of the legend.
  vtkSmartPointer<vtkBrush> Brush;

  // Description:
  // The text properties of the labels used in the legend.
  vtkSmartPointer<vtkTextProperty> LabelProperties;

  vtkTimeStamp PlotTime;
  vtkTimeStamp RectTime;

  vtkRectf Rect;
  int Padding;
  int SymbolWidth;

  // Private storage class
  class Private;
  Private* Storage;

private:
  vtkChartLegend(const vtkChartLegend &); // Not implemented.
  void operator=(const vtkChartLegend &); // Not implemented.
};

#endif //__vtkChartLegend_h
