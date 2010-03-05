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
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

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
  // Description:
  // Function to query a plot for the nearest point to the specified coordinate.
  virtual bool GetNearestPoint(const vtkVector2f& point,
                               const vtkVector2f& tolerance,
                               vtkVector2f* location);
//ETX

//BTX
  // Description:
  // Enum containing various marker styles that can be used in a plot.
  enum {
    NONE = 0,
    CROSS,
    PLUS,
    SQUARE,
    CIRCLE,
    DIAMOND
  };
//ETX

  // Description:
  // Get/set the marker style that should be used. The default is none, the enum
  // in this class is used as a parameter.
  vtkGetMacro(MarkerStyle, int);
  vtkSetMacro(MarkerStyle, int);

//BTX
protected:
  vtkPlotLine();
  ~vtkPlotLine();

  // Description:
  // Update the table cache.
  bool UpdateTableCache(vtkTable *table);

  // Description:
  // Handle calculating the log of the x or y series if necessary. Should be
  // called by UpdateTableCache once the data has been updated in Points.
  void CalculateLogSeries();

  // Description:
  // Store a well packed set of XY coordinates for this data series.
  vtkPoints2D* Points;

  // Description:
  // Sorted points, used when searching for the nearest point.
  vtkPoints2D* Sorted;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

  // Description:
  // The marker style that should be used
  int MarkerStyle;

  bool LogX, LogY;

private:
  vtkPlotLine(const vtkPlotLine &); // Not implemented.
  void operator=(const vtkPlotLine &); // Not implemented.

//ETX
};

#endif //__vtkPlotLine_h
