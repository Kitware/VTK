/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotParallelCoordinates.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPlotParallelCoordinates - Class for drawing an XY plot given two columns from a
// vtkTable.
//
// .SECTION Description
//

#ifndef __vtkPlotParallelCoordinates_h
#define __vtkPlotParallelCoordinates_h

#include "vtkPlot.h"

class vtkChartParallelCoordinates;
class vtkTable;
class vtkPoints2D;
class vtkStdString;

class VTK_CHARTS_EXPORT vtkPlotParallelCoordinates : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotParallelCoordinates, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a parallel coordinates chart
  static vtkPlotParallelCoordinates* New();

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

  // Description;
  // Set the parent, required to query the axes etc.
  virtual void SetParent(vtkChartParallelCoordinates* parent);

  // Description:
  // Set the selection criteria on the given axis in normalized space (0.0 - 1.0).
  bool SetSelectionRange(int Axis, float low, float high);

  // Description:
  // Reset the selection criteria for the chart.
  bool ResetSelectionRange();

  // Description:
  // This is a convenience function to set the input table.
  virtual void SetInput(vtkTable *table);
  virtual void SetInput(vtkTable *table, const char*, const char*)
  {
    this->SetInput(table);
  }

//BTX
protected:
  vtkPlotParallelCoordinates();
  ~vtkPlotParallelCoordinates();

  // Description:
  // Update the table cache.
  bool UpdateTableCache(vtkTable *table);

  // Description:
  // Store a well packed set of XY coordinates for this data series.
  class Private;
  Private* Storage;
  vtkPoints2D* Points;

  vtkChartParallelCoordinates* Parent;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

private:
  vtkPlotParallelCoordinates(const vtkPlotParallelCoordinates &); // Not implemented.
  void operator=(const vtkPlotParallelCoordinates &); // Not implemented.

//ETX
};

#endif //__vtkPlotParallelCoordinates_h
