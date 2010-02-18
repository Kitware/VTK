/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotLine.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPlotLine - Class for drawing an XY plot given two columns from a
// vtkTable.
//
// .SECTION Description
//

#ifndef __vtkPlotLine_h
#define __vtkPlotLine_h

#include "vtkPlot.h"

class vtkContext2D;
class vtkTable;
class vtkPoints2D;
class vtkStdString;

class VTK_CHARTS_EXPORT vtkPlotLine : public vtkPlot
{
public:
  vtkTypeRevisionMacro(vtkPlotLine, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Chart object.
  static vtkPlotLine *New();

  // Description:
  // Paint event for the XY plot, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Paint legend event for the XY plot, called whenever the legend needs the
  // plot items symbol/mark/line drawn. A rect is supplied with the lower left
  // corner of the rect (elements 0 and 1) and with width x height (elements 2
  // and 3). The plot can choose how to fill the space supplied.
  virtual bool PaintLegend(vtkContext2D *painter, float rect[4]);

  // Description:
  // Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  virtual void GetBounds(double bounds[4]);

//BTX
protected:
  vtkPlotLine();
  ~vtkPlotLine();

  // Description:
  // Update the table cache.
  bool UpdateTableCache(vtkTable *table);

  // Description:
  // Store a well packed set of XY coordinates for this data series.
  vtkPoints2D *Points;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

private:
  vtkPlotLine(const vtkPlotLine &); // Not implemented.
  void operator=(const vtkPlotLine &); // Not implemented.

//ETX
};

#endif //__vtkPlotLine_h
