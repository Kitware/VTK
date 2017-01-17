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

/**
 * @class   vtkPlotStacked
 * @brief   Class for drawing an stacked polygon plot
 * given an X, Ybase, Yextent  in a vtkTable.
 *
 *
 *
*/

#ifndef vtkPlotStacked_h
#define vtkPlotStacked_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlot.h"

class vtkChartXY;
class vtkContext2D;
class vtkTable;
class vtkPoints2D;
class vtkStdString;
class vtkImageData;
class vtkColorSeries;

class vtkPlotStackedPrivate;

class VTKCHARTSCORE_EXPORT vtkPlotStacked : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotStacked, vtkPlot);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Creates a Stacked Plot Object
   */
  static vtkPlotStacked *New();

  //@{
  /**
   * Set the plot color
   */
  void SetColor(unsigned char r, unsigned char g, unsigned char b,
                        unsigned char a) VTK_OVERRIDE;
  void SetColor(double r,  double g, double b) VTK_OVERRIDE;
  void GetColor(double rgb[3]) VTK_OVERRIDE;
  //@}

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() VTK_OVERRIDE;

  /**
   * Paint event for the Stacked plot, called whenever the chart needs to be drawn
   */
  bool Paint(vtkContext2D *painter) VTK_OVERRIDE;

  /**
   * Paint legend event for the Stacked plot, called whenever the legend needs the
   * plot items symbol/mark/line drawn. A rect is supplied with the lower left
   * corner of the rect (elements 0 and 1) and with width x height (elements 2
   * and 3). The plot can choose how to fill the space supplied.
   */
  bool PaintLegend(vtkContext2D *painter, const vtkRectf& rect,
                           int legendIndex) VTK_OVERRIDE;

  /**
   * Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax).
   */
  void GetBounds(double bounds[4]) VTK_OVERRIDE;

  /**
   * Get the unscaled input bounds for this mapper as (Xmin,Xmax,Ymin,Ymax).
   * See vtkPlot for more information.
   */
  void GetUnscaledInputBounds(double bounds[4]) VTK_OVERRIDE;

  /**
   * When used to set additional arrays, stacked bars are created.
   */
  void SetInputArray(int index, const vtkStdString &name) VTK_OVERRIDE;

  /**
   * Set the color series to use if this becomes a stacked bar plot.
   */
  void SetColorSeries(vtkColorSeries *colorSeries);

  /**
   * Get the color series used if when this is a stacked bar plot.
   */
  vtkColorSeries *GetColorSeries();

  /**
   * Get the plot labels.
   */
  vtkStringArray *GetLabels() VTK_OVERRIDE;

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated or
   * -1.
   */
  vtkIdType GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f& tolerance,
                                    vtkVector2f* location) VTK_OVERRIDE;

  /**
   * Select all points in the specified rectangle.
   */
  bool SelectPoints(const vtkVector2f& min, const vtkVector2f& max) VTK_OVERRIDE;

protected:
  vtkPlotStacked();
  ~vtkPlotStacked() VTK_OVERRIDE;

  /**
   * Update the table cache.
   */
  bool UpdateTableCache(vtkTable *table);

  // Descript:
  // For stacked plots the Extent data must be greater than (or equal to) the
  // base data. Insure that this is true
  void FixExtent();

  /**
   * Handle calculating the log of the x or y series if necessary. Should be
   * called by UpdateTableCache once the data has been updated in Points.
   */
  void CalculateLogSeries();

  /**
   * An array containing the indices of all the "bad base points", meaning any x, y
   * pair that has an infinity, -infinity or not a number value.
   */
  vtkIdTypeArray* BaseBadPoints;

  /**
   * An array containing the indices of all the "bad extent points", meaning any x, y
   * pair that has an infinity, -infinity or not a number value.
   */
  vtkIdTypeArray* ExtentBadPoints;

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  vtkTimeStamp BuildTime;

  bool LogX, LogY;

  /**
   * The color series to use for each series.
   */
  vtkSmartPointer<vtkColorSeries> ColorSeries;

private:
  vtkPlotStacked(const vtkPlotStacked &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPlotStacked &) VTK_DELETE_FUNCTION;

  vtkPlotStackedPrivate *Private;

};

#endif //vtkPlotStacked_h
