/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotFunctionalBag.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPlotFunctionalBag - Class for drawing an XY line plot or bag
// given two columns from a vtkTable.
//
// .SECTION Description
// Depending on the number of components, this class will draw either
// a line plot (for 1 component column) or, for two components columns,
// a filled polygonal band (the bag) going from the first to the second
// component on the Y-axis along the X-axis. The filter
// vtkExtractFunctionalBagPlot is intended to create such "bag" columns.
//
// .SECTION See Also
// vtkExtractFunctionalBagPlot

#ifndef vtkPlotFunctionalBag_h
#define vtkPlotFunctionalBag_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlot.h"
#include "vtkNew.h"              // Needed to hold SP ivars

class vtkDataArray;
class vtkPlotFuntionalBagInternal;
class vtkPlotLine;
class vtkPoints2D;
class vtkScalarsToColors;

class VTKCHARTSCORE_EXPORT vtkPlotFunctionalBag : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotFunctionalBag, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a functional bag plot object.
  static vtkPlotFunctionalBag *New();

  // Description:
  // Returns true if the plot is a functional bag, false if it is a simple
  // line.
  virtual bool IsBag();

  // Description:
  // Reimplemented to enforce visibility when selected.
  virtual bool GetVisible();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the plot, called whenever the chart needs to be drawn.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Paint legend event for the plot, called whenever the legend needs the
  // plot items symbol/mark/line drawn. A rect is supplied with the lower left
  // corner of the rect (elements 0 and 1) and with width x height (elements 2
  // and 3). The plot can choose how to fill the space supplied.
  virtual bool PaintLegend(vtkContext2D *painter, const vtkRectf& rect,
                           int legendIndex);

  // Description:
  // Get the bounds for this plot as (Xmin, Xmax, Ymin, Ymax).
  virtual void GetBounds(double bounds[4]);

  // Description:
  // Get the non-log-scaled bounds on chart inputs for this plot as
  // (Xmin, Xmax, Ymin, Ymax).
  virtual void GetUnscaledInputBounds(double bounds[4]);

  // Description:
  // Specify a lookup table for the mapper to use.
  void SetLookupTable(vtkScalarsToColors *lut);
  vtkScalarsToColors *GetLookupTable();

  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available with the scalar data.
  virtual void CreateDefaultLookupTable();

//BTX
  // Description:
  // Function to query a plot for the nearest point to the specified coordinate.
  // Returns the index of the data series with which the point is associated or
  // -1.
  virtual vtkIdType GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f& tolerance,
                                    vtkVector2f* location);
//ETX
  // Description:
  // Select all points in the specified rectangle.
  virtual bool SelectPoints(const vtkVector2f& min, const vtkVector2f& max);

  // Description:
  // Select all points in the specified polygon.
  virtual bool SelectPointsInPolygon(const vtkContextPolygon &polygon);
protected:
  vtkPlotFunctionalBag();
  ~vtkPlotFunctionalBag();

  // Description:
  // Populate the data arrays ready to operate on input data.
  bool GetDataArrays(vtkTable *table, vtkDataArray *array[2]);

  // Description:
  // Update the table cache.
  bool UpdateTableCache(vtkTable*);

  // Description:
  // The cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

  // Description:
  // Lookup Table for coloring points by scalar value
  vtkScalarsToColors *LookupTable;

  // Description:
  // The plot line delegate for line series
  vtkNew<vtkPlotLine> Line;

  // Description:
  // The bag points ordered in quadstrip fashion
  vtkNew<vtkPoints2D> BagPoints;

  bool LogX, LogY;

private:
  vtkPlotFunctionalBag(const vtkPlotFunctionalBag &); // Not implemented.
  void operator=(const vtkPlotFunctionalBag &); // Not implemented.
};

#endif //vtkPlotFunctionalBag_h
