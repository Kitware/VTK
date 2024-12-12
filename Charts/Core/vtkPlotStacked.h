// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkChartXY;
class vtkContext2D;
class vtkTable;
class vtkPoints2D;
class vtkStdString;
class vtkImageData;
class vtkColorSeries;

class vtkPlotStackedPrivate;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPlotStacked : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotStacked, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a Stacked Plot Object
   */
  static vtkPlotStacked* New();

  /**
   * Set the plot color with integer values (comprised between 0 and 255)
   */
  void SetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) override;
  void SetColor(unsigned char r, unsigned char g, unsigned char b) override;

  ///@{
  /**
   * Set the plot color with floating values (comprised between 0.0 and 1.0)
   */
  void SetColorF(double r, double g, double b, double a) override;
  void SetColorF(double r, double g, double b) override;
  ///@}

  ///@{
  /**
   * Get the plot color as floating rgb values (comprised between 0.0 and 1.0)
   */
  void GetColorF(double rgb[3]) override;
  ///@}

  /**
   * Paint event for the Stacked plot, called whenever the chart needs to be drawn
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Paint legend event for the Stacked plot, called whenever the legend needs the
   * plot items symbol/mark/line drawn. A rect is supplied with the lower left
   * corner of the rect (elements 0 and 1) and with width x height (elements 2
   * and 3). The plot can choose how to fill the space supplied.
   */
  bool PaintLegend(vtkContext2D* painter, const vtkRectf& rect, int legendIndex) override;

  /**
   * Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax).
   */
  void GetBounds(double bounds[4]) override;

  /**
   * Get the unscaled input bounds for this mapper as (Xmin,Xmax,Ymin,Ymax).
   * See vtkPlot for more information.
   */
  void GetUnscaledInputBounds(double bounds[4]) override;

  /**
   * When used to set additional arrays, stacked bars are created.
   */
  void SetInputArray(int index, const vtkStdString& name) override;

  /**
   * Set the color series to use if this becomes a stacked bar plot.
   */
  void SetColorSeries(vtkColorSeries* colorSeries);

  /**
   * Get the color series used if when this is a stacked bar plot.
   */
  vtkColorSeries* GetColorSeries();

  /**
   * Get the plot labels.
   */
  vtkStringArray* GetLabels() override;

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated or
   * -1.
   */
  vtkIdType GetNearestPoint(const vtkVector2f& point, const vtkVector2f& tolerance,
    vtkVector2f* location, vtkIdType* segmentId) override;
  using vtkPlot::GetNearestPoint;

  /**
   * Select all points in the specified rectangle.
   */
  bool SelectPoints(const vtkVector2f& min, const vtkVector2f& max) override;

  /**
   * Update the internal cache. Returns true if cache was successfully updated. Default does
   * nothing.
   * This method is called by Update() when either the plot's data has changed or
   * CacheRequiresUpdate() returns true. It is not necessary to call this method explicitly.
   */
  bool UpdateCache() override;

protected:
  vtkPlotStacked();
  ~vtkPlotStacked() override;

  /**
   * Test if the internal cache requires an update.
   */
  bool CacheRequiresUpdate() override;

  // Descript:
  // For stacked plots the Extent data must be greater than (or equal to) the
  // base data. Ensure that this is true
  void FixExtent();

  /**
   * Handle calculating the log of the x or y series if necessary. Should be
   * called by UpdateCache once the data has been updated in Points.
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

  bool LogX, LogY;

  /**
   * The color series to use for each series.
   */
  vtkSmartPointer<vtkColorSeries> ColorSeries;

private:
  vtkPlotStacked(const vtkPlotStacked&) = delete;
  void operator=(const vtkPlotStacked&) = delete;

  vtkPlotStackedPrivate* Private;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPlotStacked_h
