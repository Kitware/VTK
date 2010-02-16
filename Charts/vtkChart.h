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

// .NAME vtkChart - Factory class for drawing 2D charts
//
// .SECTION Description
// This defines the interface for a chart.

#ifndef __vtkChart_h
#define __vtkChart_h

#include "vtkContextItem.h"

class vtkContext2D;
class vtkContextScene;
class vtkPlot;

class vtkInteractorStyle;
class vtkAnnotationLink;
class vtkTable;

class VTK_CHARTS_EXPORT vtkChart : public vtkContextItem
{
public:
  vtkTypeRevisionMacro(vtkChart, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

//BTX
  // Description:
  // Enum of the available chart types
  enum Type {
    LINE,
    POINTS,
    BAR,
    STACKED};
//ETX

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter) = 0;

//BTX
  // Description:
  // Add a plot to the chart, defaults to using the name of the y column
  virtual vtkPlot* AddPlot(Type type);
//ETX

  // Description:
  // Remove the plot at the specified index, returns true if successful,
  // false if the index was invalid.
  virtual bool RemovePlot(vtkIdType index);

  // Description:
  // Remove all plots from the chart.
  virtual void ClearPlots();

  // Description:
  // Get the plot at the specified index, returns null if the index is invalid.
  virtual vtkPlot* GetPlot(vtkIdType index);

  // Description:
  // Get the number of plots the chart contains.
  virtual vtkIdType GetNumberPlots();

  // Description:
  // Request that the chart recalculates the range of its axes. Especially
  // useful in applications after the parameters of plots have been modified.
  virtual void RecalculateBounds();

  // Description:
  // Set the vtkAnnotationLink for the chart.
  virtual void SetAnnotationLink(vtkAnnotationLink *link);

  // Description:
  // Get the vtkAnnotationLink for the chart.
  vtkGetObjectMacro(AnnotationLink, vtkAnnotationLink);

  // Description:
  // Set/get the width and the height of the chart.
  vtkSetVector2Macro(Geometry, int);
  vtkGetVector2Macro(Geometry, int);

  // Description:
  // Set/get the first point in the chart (the bottom left).
  vtkSetVector2Macro(Point1, int);
  vtkGetVector2Macro(Point1, int);

  // Description:
  // Set/get the second point in the chart (the top right).
  vtkSetVector2Macro(Point2, int);
  vtkGetVector2Macro(Point2, int);

  // Description:
  // Set/get the borders of the chart (space in pixels around the chart).
  void SetBottomBorder(int border);
  void SetTopBorder(int border);
  void SetLeftBorder(int border);
  void SetRightBorder(int border);

  // Description:
  // Set/get the borders of the chart (space in pixels around the chart).
  void SetBorders(int left, int right, int top, int bottom);

//BTX
protected:
  vtkChart();
  ~vtkChart();

  // Description:
  // Our annotation link, used for sharing selections etc.
  vtkAnnotationLink *AnnotationLink;

  // Description:
  // The width and the height of the chart.
  int Geometry[2];

  // Description:
  // The position of the lower left corner of the chart.
  int Point1[2];

  // Description:
  // The position of the upper right corner of the chart.
  int Point2[2];

private:
  vtkChart(const vtkChart &); // Not implemented.
  void operator=(const vtkChart &);   // Not implemented.
//ETX
};

#endif //__vtkChart_h
