/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPlotStacked - Class for drawing an stacked polygon plot
// given an X, Ybase, Yextent  in a vtkTable.
//
// .SECTION Description
//

#ifndef __vtkPlotStacked_h
#define __vtkPlotStacked_h

#include "vtkPlot.h"

class vtkChartXY;
class vtkContext2D;
class vtkTable;
class vtkPoints2D;
class vtkStdString;
class vtkImageData;

class VTK_CHARTS_EXPORT vtkPlotStacked : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotStacked, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a Stacked Plot Object
  static vtkPlotStacked *New();

  // Description:
  // Set the plot color
  virtual void SetColor(unsigned char r, unsigned char g, unsigned char b,
                        unsigned char a);
  virtual void SetColor(double r,  double g, double b);
  virtual void GetColor(double rgb[3]);

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the Stacked plot, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Paint legend event for the Stacked plot, called whenever the legend needs the
  // plot items symbol/mark/line drawn. A rect is supplied with the lower left
  // corner of the rect (elements 0 and 1) and with width x height (elements 2
  // and 3). The plot can choose how to fill the space supplied.
  virtual bool PaintLegend(vtkContext2D *painter, float rect[4], int legendIndex);

  // Description:
  // Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  virtual void GetBounds(double bounds[4]);


//BTX
  // Description:
  // Function to query a plot for the nearest point to the specified coordinate.
  // Returns the index of the data series with which the point is associated or 
  // -1.
  virtual int GetNearestPoint(const vtkVector2f& point,
                               const vtkVector2f& tolerance,
                               vtkVector2f* location);

  // Description:
  // Select all points in the specified rectangle.
  virtual bool SelectPoints(const vtkVector2f& min, const vtkVector2f& max);

  // Description:
  // Set the parent, required to accumlate base points when positioning is implicit
  virtual void SetParent(vtkChartXY *parent);

  // Description:
  // Make this plot visible or invisible
  virtual void SetVisible(bool visible);
    

//BTX
protected:
  vtkPlotStacked();
  ~vtkPlotStacked();

  // Description:
  // Update the table cache.
  bool UpdateTableCache(vtkTable *table);

  // Descript:
  // For stacked plots the Extent data must be greater than (or equal to) the 
  // base data. Insure that this is true
  void FixExtent();

  // Description:
  // Handle calculating the log of the x or y series if necessary. Should be
  // called by UpdateTableCache once the data has been updated in Points.
  void CalculateLogSeries();

  // Description:
  // Find all of the "bad points" in the series. This is mainly used to cache
  // bad points for performance reasons, but could also be used plot the bad
  // points in the future.
  void FindBadPoints();

  // Description:
  // Calculate the bounds of the plot, ignoring the bad points.
  void CalculateBounds(vtkPoints2D *points, vtkIdTypeArray *badPoints, double bounds[4]);

  // Description:
  // Store a well packed set of XY coordinates for the base of this series
  vtkPoints2D *BasePoints;
  // Description:
  // Store a well packed set of XY coordinates for the extent of this series
  vtkPoints2D *ExtentPoints;

  // Description:
  // Sorted points, used when searching for the nearest point.
  class VectorPIMPL;
  VectorPIMPL* Sorted;

  // Description:
  // An array containing the indices of all the "bad base points", meaning any x, y
  // pair that has an infinity, -infinity or not a number value.
  vtkIdTypeArray* BaseBadPoints;

  // Description:
  // An array containing the indices of all the "bad extent points", meaning any x, y
  // pair that has an infinity, -infinity or not a number value.
  vtkIdTypeArray* ExtentBadPoints;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

  bool LogX, LogY;

  // Description:
  // The parent Chart of this Plot
  vtkChartXY *Parent;

private:
  vtkPlotStacked(const vtkPlotStacked &); // Not implemented.
  void operator=(const vtkPlotStacked &); // Not implemented.

//ETX
};

#endif //__vtkPlotStacked_h
