/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotBag.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPlotBag - Class for drawing an a bagplot.
//
// .SECTION Description
// This class allows to draw a bagplot given three columns from
// a vtkTable. The first two columns will represent X,Y as it is for
// vtkPlotPoints. The third one will have to specify if the density
// assigned to each point (generally obtained by the
// vtkHighestDensityRegionsStatistics filter).
// Points are drawn in a plot points fashion and 2 convex hull polygons
// are drawn around the median and the 3 quartile of the density field.
//
// .SECTION See Also
// vtkHighestDensityRegionsStatistics

#ifndef __vtkPlotBag_h
#define __vtkPlotBag_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlotPoints.h"

class VTKCHARTSCORE_EXPORT vtkPlotBag : public vtkPlotPoints
{
public:
  vtkTypeMacro(vtkPlotBag, vtkPlotPoints);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a new Bag Plot object.
  static vtkPlotBag *New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the XY plot, called whenever the chart needs to be drawn.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Paint legend event for the XY plot, called whenever the legend needs the
  // plot items symbol/mark/line drawn. A rect is supplied with the lower left
  // corner of the rect (elements 0 and 1) and with width x height (elements 2
  // and 3). The plot can choose how to fill the space supplied.
  virtual bool PaintLegend(vtkContext2D *painter, const vtkRectf& rect,
                           int legendIndex);

  // Description:
  // Get the plot labels. If this array has a length greater than 1 the index
  // refers to the stacked objects in the plot. See vtkPlotBar for example.
  virtual vtkStringArray *GetLabels();

  // Description:
  // Generate and return the tooltip label string for this plot
  // The segmentIndex parameter is ignored, except for vtkPlotBar
  virtual vtkStdString GetTooltipLabel(const vtkVector2d &plotPos,
                                       vtkIdType seriesIndex,
                                       vtkIdType segmentIndex);

  // Description:
  // Set the input, we are expecting a vtkTable with three columns. The first
  // column and the second represent the x,y position . The five others
  // columns represent the quartiles used to display the box.
  // Inherited method will call the last SetInputData method with default
  // paramaters.
  virtual void SetInputData(vtkTable *table);
  virtual void SetInputData(vtkTable *table, const vtkStdString &yColumn,
                            const vtkStdString &densityColumn);
  virtual void SetInputData(vtkTable *table, const vtkStdString &xColumn,
                            const vtkStdString &yColumn,
                            const vtkStdString &densityColumn);

  virtual void SetInputData(vtkTable *table, vtkIdType xColumn,
                            vtkIdType yColumn,
                            vtkIdType densityColumn);

protected:
  vtkPlotBag();
  ~vtkPlotBag();

  void UpdateTableCache(vtkDataArray*);

  vtkPoints2D* MedianPoints;
  vtkPoints2D* Q3Points;

private:
  vtkPlotBag(const vtkPlotBag &); // Not implemented.
  void operator=(const vtkPlotBag &); // Not implemented.
};

#endif //__vtkPlotBag_h
