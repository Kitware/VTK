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

  // Description:
  // Add a plot to the chart, defaults to using the name of the y column
  virtual vtkPlot * AddPlot(Type type);

  // Description:
  // Get the number of plots the chart contains.
  virtual vtkIdType GetNumberPlots();

  // Description:
  // Set the vtkAnnotationLink for the chart.
  virtual void SetAnnotationLink(vtkAnnotationLink *link);

  // Description:
  // Get the vtkAnnotationLink for the chart.
  vtkGetObjectMacro(AnnotationLink, vtkAnnotationLink);

  // Description:
  // This function allows you to set the overall dimensions of the chart.
  // An int pointer of length 6 is expected with the dimensions in the order of
  // width, height, left border, bottom border, right border, top border in
  // pixels of the device.
  vtkSetVector6Macro(Geometry, int);

  // Description:
  // Add the chart as an observer on the supplied interaction style.
  void AddInteractorStyle(vtkInteractorStyle *interactor);

//BTX
protected:
  vtkChart();
  ~vtkChart();

  // Description:
  // Called to process events.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId,
                             void* callData);

  // Description:
  // Process a rubber band selection event.
  virtual void ProcessSelectionEvent(vtkObject* caller, void* callData);

  // Description:
  // Our annotation link, used for sharing selections etc.
  vtkAnnotationLink *AnnotationLink;

  // Store the chart dimensions packed into a vtkPoints2D
  // [0] = width, height of chart in screen coordinates
  // [1] = left border, bottom border (roughly - origin of the chart
  // [2] = right border, top border (offset from top right most point)
  int Geometry[6];

  // Description:
  // The command object for the charts.
  class Command;
  friend class Command;
  Command *Observer;

private:
  vtkChart(const vtkChart &); // Not implemented.
  void operator=(const vtkChart &);   // Not implemented.
//ETX
};

#endif //__vtkChart_h
