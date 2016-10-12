/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotArea.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class VTKCHARTSCORE_EXPORT vtkPlotArea : public vtkPlot
{
public:
  static vtkPlotArea* New();
  vtkTypeMacro(vtkPlotArea, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Convenience method to set the input arrays. vtkPlotArea supports the
   * following indices:
   * \li 0: x-axis,
   * \li 1: y-axis,
   * \li 2: y-axis.
   */
  using Superclass::SetInputArray;

  //@{
  /**
   * Overridden to set the brush color.
   */
  virtual void SetColor(unsigned char r, unsigned char g, unsigned char b,
                        unsigned char a);
  virtual void SetColor(double r,  double g, double b);
  //@}

  //@{
  /**
   * Get/set the valid point mask array name.
   */
  vtkGetMacro(ValidPointMaskName, vtkStdString)
  vtkSetMacro(ValidPointMaskName, vtkStdString)
  //@}

  /**
   * Perform any updates to the item that may be necessary before rendering.
   */
  virtual void Update();

  /**
   * Get the bounds for this plot as (Xmin, Xmax, Ymin, Ymax).
   */
  virtual void GetBounds(double bounds[4]);

  /**
   * Subclasses that build data caches to speed up painting should override this
   * method to update such caches. This is called on each Paint, hence
   * subclasses must add checks to avoid rebuilding of cache, unless necessary.
   */
  virtual void UpdateCache();

  /**
   * Paint event for the XY plot, called whenever the chart needs to be drawn
   */
  virtual bool Paint(vtkContext2D *painter);

  /**
   * Paint legend event for the plot, called whenever the legend needs the
   * plot items symbol/mark/line drawn. A rect is supplied with the lower left
   * corner of the rect (elements 0 and 1) and with width x height (elements 2
   * and 3). The plot can choose how to fill the space supplied. The index is used
   * by Plots that return more than one label.
   */
  virtual bool PaintLegend(vtkContext2D *painter, const vtkRectf& rect,
                           int legendIndex);

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated, or
   * -1 if no point was found.
   */
  virtual vtkIdType GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f& tolerance,
                                    vtkVector2f* location);

  /**
   * Generate and return the tooltip label string for this plot
   * The segmentIndex parameter is ignored, except for vtkPlotBar
   */
  virtual vtkStdString GetTooltipLabel(const vtkVector2d &plotPos,
                                       vtkIdType seriesIndex,
                                       vtkIdType segmentIndex);

protected:
  vtkPlotArea();
  ~vtkPlotArea();

  /**
   * Name of the valid point mask array.
   */
  vtkStdString ValidPointMaskName;

private:
  vtkPlotArea(const vtkPlotArea&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPlotArea&) VTK_DELETE_FUNCTION;

  class vtkTableCache;
  vtkTableCache* TableCache;

  vtkTimeStamp UpdateTime;

};

#endif
