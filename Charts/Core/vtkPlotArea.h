// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPlotArea
 * @brief   draws an area plot.
 *
 * vtkPlotArea is used to render an area plot. An area plot (sometimes called a
 * range plot) renders a filled region between the selected ymin and ymax
 * arrays.
 * To specify the x array and ymin/ymax arrays, use the SetInputArray method
 * with array index as 0, 1, or 2, respectively.
 */

#ifndef vtkPlotArea_h
#define vtkPlotArea_h

#include "vtkPlot.h"

#include "vtkChartsCoreModule.h" // for export macro
#include "vtkWrappingHints.h"    // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPlotArea : public vtkPlot
{
public:
  static vtkPlotArea* New();
  vtkTypeMacro(vtkPlotArea, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Convenience method to set the input arrays. vtkPlotArea supports the
   * following indices:
   * \li 0: x-axis,
   * \li 1: y-axis,
   * \li 2: y-axis.
   */
  using Superclass::SetInputArray;

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
   * Get/set the valid point mask array name.
   */
  vtkGetMacro(ValidPointMaskName, vtkStdString);
  vtkSetMacro(ValidPointMaskName, vtkStdString);
  ///@}

  /**
   * Get the bounds for this plot as (Xmin, Xmax, Ymin, Ymax).
   */
  void GetBounds(double bounds[4]) override;

  /**
   * Paint event for the XY plot, called whenever the chart needs to be drawn
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Paint legend event for the plot, called whenever the legend needs the
   * plot items symbol/mark/line drawn. A rect is supplied with the lower left
   * corner of the rect (elements 0 and 1) and with width x height (elements 2
   * and 3). The plot can choose how to fill the space supplied. The index is used
   * by Plots that return more than one label.
   */
  bool PaintLegend(vtkContext2D* painter, const vtkRectf& rect, int legendIndex) override;

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated, or
   * -1 if no point was found.
   */
  vtkIdType GetNearestPoint(const vtkVector2f& point, const vtkVector2f& tolerance,
    vtkVector2f* location, vtkIdType* segmentId) override;
  using vtkPlot::GetNearestPoint;

  /**
   * Generate and return the tooltip label string for this plot
   * The segmentIndex parameter is ignored, except for vtkPlotBar
   */
  vtkStdString GetTooltipLabel(
    const vtkVector2d& plotPos, vtkIdType seriesIndex, vtkIdType segmentIndex) override;

  /**
   * Update the internal cache. Returns true if cache was successfully updated. Default does
   * nothing.
   * This method is called by Update() when either the plot's data has changed or
   * CacheRequiresUpdate() returns true. It is not necessary to call this method explicitly.
   */
  bool UpdateCache() override;

protected:
  vtkPlotArea();
  ~vtkPlotArea() override;

  /**
   * Name of the valid point mask array.
   */
  vtkStdString ValidPointMaskName;

private:
  vtkPlotArea(const vtkPlotArea&) = delete;
  void operator=(const vtkPlotArea&) = delete;

  class vtkTableCache;
  vtkTableCache* TableCache;

  vtkTimeStamp UpdateTime;
};

VTK_ABI_NAMESPACE_END
#endif
