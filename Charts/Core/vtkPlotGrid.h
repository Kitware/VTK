/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPlotGrid - takes care of drawing the plot grid
//
// .SECTION Description
// The vtkPlotGrid is drawn in screen coordinates. It is usually one of the
// first elements of a chart to be drawn, and will generally be obscured
// by all other elements of the chart. It builds up its own plot locations
// from the parameters of the x and y axis of the plot.

#ifndef __vtkPlotGrid_h
#define __vtkPlotGrid_h

#include "vtkContextItem.h"

class vtkStdString;
class vtkContext2D;
class vtkPoints2D;
class vtkAxis;

class VTK_CHARTS_EXPORT vtkPlotGrid : public vtkContextItem
{
public:
  vtkTypeMacro(vtkPlotGrid, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Chart object.
  static vtkPlotGrid *New();

  // Description:
  // Set the X axis of the grid.
  virtual void SetXAxis(vtkAxis *axis);

  // Description:
  // Set the X axis of the grid.
  virtual void SetYAxis(vtkAxis *axis);

  // Description:
  // Paint event for the axis, called whenever the axis needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

//BTX
protected:
  vtkPlotGrid();
  ~vtkPlotGrid();

  // Description:
  // The vtkAxis objects are used to figure out where the grid lines should be
  // drawn.
  vtkAxis *XAxis;
  vtkAxis *YAxis;

private:
  vtkPlotGrid(const vtkPlotGrid &); // Not implemented.
  void operator=(const vtkPlotGrid &);   // Not implemented.
//ETX
};

#endif //__vtkPlotGrid_h
