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

class vtkVector2f;
class vtkChart;

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

//BTX
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
//ETX

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
  // Set the point size of the label text.
  vtkSetMacro(LabelSize, int);

  // Description:
  // Get the point size of the label text.
  vtkGetMacro(LabelSize, int);

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

//BTX
protected:
  vtkChartLegend();
  ~vtkChartLegend();

  float* Point;  // The point the legend is anchored to.
  int HorizontalAlignment; // Alignment of the legend to the point it is anchored to.
  int VerticalAlignment; // Alignment of the legend to the point it is anchored to.
  int LabelSize; // The point size of the labels

  // Private storage class
  class Private;
  Private* Storage;

private:
  vtkChartLegend(const vtkChartLegend &); // Not implemented.
  void operator=(const vtkChartLegend &); // Not implemented.
//ETX
};

#endif //__vtkChartLegend_h
