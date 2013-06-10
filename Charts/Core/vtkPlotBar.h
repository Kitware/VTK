/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotBar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPlotBar - Class for drawing an XY plot given two columns from a
// vtkTable.
//
// .SECTION Description
//

#ifndef __vtkPlotBar_h
#define __vtkPlotBar_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlot.h"
#include "vtkSmartPointer.h" // Needed to hold ColorSeries

class vtkContext2D;
class vtkTable;
class vtkPoints2D;
class vtkStdString;
class vtkColorSeries;

class vtkPlotBarPrivate;

class VTKCHARTSCORE_EXPORT vtkPlotBar : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotBar, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Enum of bar chart oritentation types
  enum {
    VERTICAL = 0,
    HORIZONTAL
  };

  // Description:
  // Creates a 2D Chart object.
  static vtkPlotBar *New();

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
  // Set the plot color
  virtual void SetColor(unsigned char r, unsigned char g, unsigned char b,
                        unsigned char a);
  virtual void SetColor(double r,  double g, double b);
  virtual void GetColor(double rgb[3]);

  // Description:
  // Set the width of the line.
  vtkSetMacro(Width, float);

  // Description:
  // Get the width of the line.
  vtkGetMacro(Width, float);

  // Description:
  // Set/get the horizontal offset of the bars.
  // Positive values move the bars leftward.
  // For HORIZONTAL orientation, offsets bars vertically,
  // with a positive value moving bars downward.
  vtkSetMacro(Offset, float);
  vtkGetMacro(Offset, float);

  // Description:
  // Set/get the orientation of the bars.
  // Valid orientations are VERTICAL (default) and HORIZONTAL.
  virtual void SetOrientation(int orientation);
  vtkGetMacro(Orientation, int);

  // Description:
  // A helper used by both GetUnscaledBounds and GetBounds(double[4]).
  virtual void GetBounds(double bounds[4], bool unscaled);

  // Description:
  // Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax).
  virtual void GetBounds(double bounds[4]);

  // Description:
  // Get un-log-scaled bounds for this mapper as (Xmin,Xmax,Ymin,Ymax).
  virtual void GetUnscaledInputBounds(double bounds[4]);

  // Description:
  // When used to set additional arrays, stacked bars are created.
  virtual void SetInputArray(int index, const vtkStdString &name);

  // Description:
  // Set the color series to use if this becomes a stacked bar plot.
  void SetColorSeries(vtkColorSeries *colorSeries);

  // Description:
  // Get the color series used if when this is a stacked bar plot.
  vtkColorSeries *GetColorSeries();

  // Description
  // Get the plot labels.
  virtual vtkStringArray *GetLabels();

  // Description:
  // Set the group name of the bar chart - can be displayed on the X axis.
  virtual void SetGroupName(const vtkStdString& name);

  // Description:
  // Get the group name of the bar char - can be displayed on the X axis.
  virtual vtkStdString GetGroupName();

  // Description:
  // Generate and return the tooltip label string for this plot
  // The segmentIndex is implemented here.
  virtual vtkStdString GetTooltipLabel(const vtkVector2f &plotPos,
                                       vtkIdType seriesIndex,
                                       vtkIdType segmentIndex);

  // Description:
  // Select all points in the specified rectangle.
  virtual bool SelectPoints(const vtkVector2f& min, const vtkVector2f& max);

//BTX
  // Description:
  // Function to query a plot for the nearest point to the specified coordinate.
  // Returns the index of the data series with which the point is associated or
  // -1.
  virtual vtkIdType GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f& tolerance,
                                    vtkVector2f* location);

  // Description:
  // Function to query a plot for the nearest point to the specified coordinate.
  // Returns the index of the data series with which the point is associated or
  // -1.
  // If a vtkIdType* is passed, its referent will be set to index of the bar
  // segment with which a point is associated, or -1.
  virtual vtkIdType GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f&,
                                    vtkVector2f* location,
                                    vtkIdType* segmentIndex);

protected:
  vtkPlotBar();
  ~vtkPlotBar();

  // Description:
  // Update the table cache.
  bool UpdateTableCache(vtkTable *table);

  // Description:
  // Store a well packed set of XY coordinates for this data series.
  vtkPoints2D *Points;

  float Width;
  float Offset;

  int Orientation;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

  // Description:
  // The color series to use if this becomes a stacked bar
  vtkSmartPointer<vtkColorSeries> ColorSeries;

private:
  vtkPlotBar(const vtkPlotBar &); // Not implemented.
  void operator=(const vtkPlotBar &); // Not implemented.

  vtkPlotBarPrivate *Private;

//ETX
};

#endif //__vtkPlotBar_h
