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

#include "vtkPlot.h"
#include "vtkSmartPointer.h" // Needed to hold ColorSeries

class vtkContext2D;
class vtkTable;
class vtkPoints2D;
class vtkStdString;
class vtkColorSeries;

class vtkPlotBarPrivate;

class VTK_CHARTS_EXPORT vtkPlotBar : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotBar, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

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
  virtual bool PaintLegend(vtkContext2D *painter, float rect[4], int legendIndex);

  // Description:
  // Set the plot color
  virtual void SetColor(unsigned char r, unsigned char g, unsigned char b,
                        unsigned char a);
  virtual void SetColor(double r,  double g, double b);
  virtual void GetColor(double rgb[3]);

  // Description:
  // Set the width of the line.
  virtual void SetWidth(float width);

  // Description:
  // Get the width of the line.
  virtual float GetWidth();

  vtkSetMacro(Offset, float);
  vtkGetMacro(Offset, float);

  // Description:
  // Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax).
  virtual void GetBounds(double bounds[4]);

  // Description:
  // When used to set additional arrays, stacked bars are created.
  virtual void SetInputArray(int index, const char *name);

  // Description:
  // Set the color series to use if this becomes a stacked bar plot.
  void SetColorSeries(vtkColorSeries *colorSeries);

  // Description:
  // Get the color series used if when this is a stacked bar plot.
  vtkColorSeries *GetColorSeries();

  // Description
  // Get the plot labels.
  virtual vtkStringArray *GetLabels();

//BTX
  // Description:
  // Function to query a plot for the nearest point to the specified coordinate.
  // Returns the index of the data series with which the point is associated or 
  // -1.
  virtual int GetNearestPoint(const vtkVector2f& point,
                               const vtkVector2f& tolerance,
                               vtkVector2f* location);
//ETX

//BTX
protected:
  vtkPlotBar();
  ~vtkPlotBar();

  // Description:
  // Update the table cache.
  bool UpdateTableCache(vtkTable *table);

  // Description:
  // Store a well packed set of XY coordinates for this data series.
  vtkPoints2D *Points;

  bool Sorted;

  float Width;
  float Offset;

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
