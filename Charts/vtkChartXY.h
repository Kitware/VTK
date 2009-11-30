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

// .NAME vtkChartXY - Factory class for drawing XY charts
//
// .SECTION Description
// This class implements an XY chart.

#ifndef __vtkChartXY_h
#define __vtkChartXY_h

#include "vtkChart.h"

class vtkPlot;
class vtkAxis;
class vtkPlotGrid;
class vtkTable;
class vtkChartXYPrivate; // Private class to keep my STL vector in...

class VTK_CHARTS_EXPORT vtkChartXY : public vtkChart
{
public:
  vtkTypeRevisionMacro(vtkChartXY, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

//BTX
  // Description:
  // Enum of the available chart types
  enum Type {
    LINE,
    STACKED};
//ETX

  // Description:
  // Creates a 2D Chart object.
  static vtkChartXY *New();

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Add a plot to the chart, defaults to using the name of the y column
//BTX
  virtual vtkPlot * AddPlot(vtkChart::Type type);
//ETX
  virtual vtkIdType GetNumberPlots();

//BTX
protected:
  vtkChartXY();
  ~vtkChartXY();

  // Description:
  // Process a rubber band selection event.
  virtual void ProcessSelectionEvent(vtkObject* caller, void* callData);

  // The X and Y axes for the chart
  vtkAxis *XAxis, *YAxis;
  vtkPlotGrid *Grid;

private:
  vtkChartXY(const vtkChartXY &); // Not implemented.
  void operator=(const vtkChartXY &);   // Not implemented.

  vtkChartXYPrivate *ChartPrivate; // Private class where I hide my STL containers

  // Private functions to render different parts of the chart
  void RenderPlots(vtkContext2D *painter);

//ETX
};

#endif //__vtkChartXY_h
