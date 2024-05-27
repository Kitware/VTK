// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPlotBox
 * @brief   Class for drawing box plots.
 *
 *
 * Plots to draw box plots given columns from a vtkTable that may contain
 * 5 lines with quartiles and median.
 */

#ifndef vtkPlotBox_h
#define vtkPlotBox_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlot.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkBrush;
class vtkTextProperty;
class vtkTable;
class vtkStdString;
class vtkScalarsToColors;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPlotBox : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotBox, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a box plot.
   */
  static vtkPlotBox* New();

  /**
   * Paint event for the plot, called whenever the chart needs to be drawn
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Paint legend event for the plot, called whenever the legend needs the
   * plot items symbol/mark/line drawn. A rect is supplied with the lower left
   * corner of the rect (elements 0 and 1) and with width x height (elements 2
   * and 3). The plot can choose how to fill the space supplied.
   */
  bool PaintLegend(vtkContext2D* painter, const vtkRectf& rect, int legendIndex) override;

  ///@{
  /**
   * This is a convenience function to set the input table.
   */
  void SetInputData(vtkTable* table) override;
  void SetInputData(vtkTable* table, const vtkStdString&, const vtkStdString&) override
  {
    this->SetInputData(table);
  }
  ///@}

  /**
   * Get the plot labels. If this array has a length greater than 1 the index
   * refers to the stacked objects in the plot.
   */
  vtkStringArray* GetLabels() override;

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated
   * or -1.
   */
  vtkIdType GetNearestPoint(const vtkVector2f& point, const vtkVector2f& tolerance,
    vtkVector2f* location, vtkIdType* segmentId) override;
  using vtkPlot::GetNearestPoint;

  ///@{
  /**
   * Specify a lookup table for the mapper to use.
   */
  void SetLookupTable(vtkScalarsToColors* lut);
  vtkScalarsToColors* GetLookupTable();
  ///@}

  /**
   * Helper function to set the color of a given column.
   */
  void SetColumnColor(const vtkStdString& colName, double* rgb);

  /**
   * Create default lookup table. Generally used to create one when none
   * is available with the scalar data.
   */
  virtual void CreateDefaultLookupTable();

  ///@{
  /**
   * Get/Set the width of boxes.
   */
  vtkGetMacro(BoxWidth, float);
  vtkSetMacro(BoxWidth, float);
  ///@}

  ///@{
  /**
   * Get the vtkTextProperty that governs how the plot title is displayed.
   */
  vtkGetObjectMacro(TitleProperties, vtkTextProperty);
  ///@}

  /**
   * Update the internal cache. Returns true if cache was successfully updated. Default does
   * nothing.
   * This method is called by Update() when either the plot's data has changed or
   * CacheRequiresUpdate() returns true. It is not necessary to call this method explicitly.
   */
  bool UpdateCache() override;

protected:
  vtkPlotBox();
  ~vtkPlotBox() override;

  void DrawBoxPlot(int, unsigned char*, double, vtkContext2D*);

  ///@{
  /**
   * Store a well packed set of XY coordinates for this data series.
   */
  class Private;
  Private* Storage;
  ///@}

  /**
   * Width of boxes.
   */
  float BoxWidth;

  /**
   * Lookup Table for coloring points by scalar value
   */
  vtkScalarsToColors* LookupTable;

  /**
   * Text properties for the plot title
   */
  vtkTextProperty* TitleProperties;

private:
  vtkPlotBox(const vtkPlotBox&) = delete;
  void operator=(const vtkPlotBox&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPlotBox_h
