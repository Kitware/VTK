/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotPie.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPlotPie - Class for drawing a Pie diagram.
//
// .SECTION Description

#ifndef __vtkPlotPie_h
#define __vtkPlotPie_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlot.h"
#include "vtkSmartPointer.h" // To hold ColorSeries etc.

class vtkContext2D;
class vtkColorSeries;
class vtkPoints2D;

class vtkPlotPiePrivate;

class VTKCHARTSCORE_EXPORT vtkPlotPie : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotPie, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkPlotPie *New();

  // Description:
  // Paint event for the item.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Paint legend event for the XY plot, called whenever the legend needs the
  // plot items symbol/mark/line drawn. A rect is supplied with the lower left
  // corner of the rect (elements 0 and 1) and with width x height (elements 2
  // and 3). The plot can choose how to fill the space supplied.
  bool PaintLegend(vtkContext2D *painter, const vtkRectf& rect, int legendIndex);

  // Description:
  // Set the dimensions of the pie, arguments 1 and 2 are the x and y coordinate
  // of the bottom corner. Arguments 3 and 4 are the width and height.
  void SetDimensions(int arg1, int arg2, int arg3, int arg4);

  // Description:
  // Set the dimensions of the pie, elements 0 and 1 are the x and y coordinate
  // of the bottom corner. Elements 2 and 3 are the width and height.
  void SetDimensions(int arg[4]);

  // Description:
  // Get the dimensions of the pie, elements 0 and 1 are the x and y coordinate
  // of the bottom corner. Elements 2 and 3 are the width and height.
  vtkGetVector4Macro(Dimensions, int);

  // Description:
  // Set the color series to use for the Pie.
  void SetColorSeries(vtkColorSeries *colorSeries);

  // Description:
  // Get the color series used.
  vtkColorSeries *GetColorSeries();

//BTX
  // Description:
  // Function to query a plot for the nearest point to the specified coordinate.
  // Returns the index of the data series with which the point is associated or
  // -1.
  virtual vtkIdType GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f& tolerance,
                                    vtkVector2f* location);

protected:
  vtkPlotPie();
  ~vtkPlotPie();

  // Description:
  // Update the table cache.
  bool UpdateTableCache(vtkTable *table);

  int Dimensions[4];

  // Description:
  // The color series to use for the pie.
  vtkSmartPointer<vtkColorSeries> ColorSeries;

  // Description:
  // Store a well packed set of angles for the wedges of the pie.
  vtkPoints2D *Points;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

private:
  vtkPlotPie(const vtkPlotPie &);     // Not implemented.
  void operator=(const vtkPlotPie &); // Not implemented.

  vtkPlotPiePrivate *Private;
//ETX
};

#endif //__vtkPlotPie_h
