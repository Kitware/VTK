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

// .NAME vtkPlotPoints - Class for drawing an points given two columns from a
// vtkTable.
//
// .SECTION Description
// This class draws points in a plot given two columns from a table. If you need
// a line as well you should use vtkPlotLine which derives from vtkPlotPoints
// and is capable of drawing both points and a line.
//
// .SECTION See Also
// vtkPlotLine

#ifndef __vtkPlotPoints_h
#define __vtkPlotPoints_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlot.h"
#include "vtkScalarsToColors.h" // For VTK_COLOR_MODE_DEFAULT and _MAP_SCALARS
#include "vtkStdString.h"       // For color array name
#include "vtkNew.h"             // For ivars
#include "vtkRenderingCoreEnums.h" // For marker enum

class vtkContext2D;
class vtkTable;
class vtkPoints2D;
class vtkFloatArray;
class vtkStdString;
class vtkImageData;
class vtkScalarsToColors;
class vtkUnsignedCharArray;

class VTKCHARTSCORE_EXPORT vtkPlotPoints : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotPoints, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Chart object.
  static vtkPlotPoints *New();

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
  virtual bool PaintLegend(vtkContext2D *painter, const vtkRectf& rect,
                           int legendIndex);

  // Description:
  // Get the bounds for this plot as (Xmin, Xmax, Ymin, Ymax).
  virtual void GetBounds(double bounds[4]);

  // Description:
  // Specify a lookup table for the mapper to use.
  void SetLookupTable(vtkScalarsToColors *lut);
  vtkScalarsToColors *GetLookupTable();

  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available with the scalar data.
  virtual void CreateDefaultLookupTable();

  // Description:
  // Turn on/off flag to control whether scalar data is used to color objects.
  vtkSetMacro(ScalarVisibility,int);
  vtkGetMacro(ScalarVisibility,int);
  vtkBooleanMacro(ScalarVisibility,int);

  // Description:
  // When ScalarMode is set to UsePointFieldData or UseCellFieldData,
  // you can specify which array to use for coloring using these methods.
  // The lookup table will decide how to convert vectors to colors.
  void SelectColorArray(vtkIdType arrayNum);
  void SelectColorArray(const vtkStdString& arrayName);

  // Description:
  // Get the array name to color by.
  vtkStdString GetColorArrayName();

//BTX
  // Description:
  // Function to query a plot for the nearest point to the specified coordinate.
  // Returns the index of the data series with which the point is associated or
  // -1.
  virtual vtkIdType GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f& tolerance,
                                    vtkVector2f* location);

  // Description:
  // Select all points in the specified rectangle.
  virtual bool SelectPoints(const vtkVector2f& min, const vtkVector2f& max);

  // Description:
  // Select all points in the specified polygon.
  virtual bool SelectPointsInPolygon(const vtkContextPolygon &polygon);

  // Description:
  // Enum containing various marker styles that can be used in a plot.
  enum {
    NONE = VTK_MARKER_NONE,
    CROSS = VTK_MARKER_CROSS,
    PLUS = VTK_MARKER_PLUS,
    SQUARE = VTK_MARKER_SQUARE,
    CIRCLE = VTK_MARKER_CIRCLE,
    DIAMOND = VTK_MARKER_DIAMOND
  };
//ETX

  // Description:
  // Get/set the marker style that should be used. The default is none, the enum
  // in this class is used as a parameter.
  vtkGetMacro(MarkerStyle, int);
  vtkSetMacro(MarkerStyle, int);

  // Description:
  // Get/set the marker size that should be used. The default is negative, and
  // in that case it is 2.3 times the pen width, if less than 8 will be used.
  vtkGetMacro(MarkerSize, float);
  vtkSetMacro(MarkerSize, float);

//BTX
protected:
  vtkPlotPoints();
  ~vtkPlotPoints();

  // Description:
  // Update the table cache.
  bool UpdateTableCache(vtkTable *table);

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
  void CalculateBounds(double bounds[4]);

  // Description:
  // Create the sorted point list if necessary.
  void CreateSortedPoints();

  // Description:
  // Store a well packed set of XY coordinates for this data series.
  vtkPoints2D *Points;
  vtkNew<vtkFloatArray> SelectedPoints;

  // Description:
  // Sorted points, used when searching for the nearest point.
  class VectorPIMPL;
  VectorPIMPL* Sorted;

  // Description:
  // An array containing the indices of all the "bad points", meaning any x, y
  // pair that has an infinity, -infinity or not a number value.
  vtkIdTypeArray* BadPoints;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

  // Description:
  // The marker style that should be used
  int MarkerStyle;
  float MarkerSize;

  bool LogX, LogY;

  // Description:
  // Lookup Table for coloring points by scalar value
  vtkScalarsToColors *LookupTable;
  vtkUnsignedCharArray *Colors;
  int ScalarVisibility;
  vtkStdString ColorArrayName;

private:
  vtkPlotPoints(const vtkPlotPoints &); // Not implemented.
  void operator=(const vtkPlotPoints &); // Not implemented.

// #define  VTK_COLOR_MODE_DEFAULT   0
// #define  VTK_COLOR_MODE_MAP_SCALARS   1
//ETX
};

#endif //__vtkPlotPoints_h
