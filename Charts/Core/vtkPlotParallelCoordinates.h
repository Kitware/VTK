// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPlotParallelCoordinates
 * @brief   Class for drawing a parallel coordinate
 * plot given columns from a vtkTable.
 *
 *
 *
 */

#ifndef vtkPlotParallelCoordinates_h
#define vtkPlotParallelCoordinates_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlot.h"
#include "vtkScalarsToColors.h" // For VTK_COLOR_MODE_DEFAULT and _MAP_SCALARS
#include "vtkStdString.h"       // For vtkStdString ivars
#include "vtkWrappingHints.h"   // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkChartParallelCoordinates;
class vtkScalarsToColors;
class vtkTable;
class vtkUnsignedCharArray;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPlotParallelCoordinates : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotParallelCoordinates, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a parallel coordinates chart
   */
  static vtkPlotParallelCoordinates* New();

  /**
   * Paint event for the XY plot, called whenever the chart needs to be drawn
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Paint legend event for the XY plot, called whenever the legend needs the
   * plot items symbol/mark/line drawn. A rect is supplied with the lower left
   * corner of the rect (elements 0 and 1) and with width x height (elements 2
   * and 3). The plot can choose how to fill the space supplied.
   */
  bool PaintLegend(vtkContext2D* painter, const vtkRectf& rect, int legendIndex) override;

  /**
   * Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  void GetBounds(double bounds[4]) override;

  /**
   * Set the selection criteria on the given axis in normalized space (0.0 - 1.0) for a specific
   * range.
   */
  bool SetSelectionRange(int axis, float low, float high);

  /**
   * Set the selection criteria on the given axis in normalized space [0.0 ; 1.0]
   * axisSelection should be a list like {minRange1, maxRange1, minRange2, maxRange2, ...}
   */
  bool SetSelectionRange(int axis, std::vector<float> axisSelection);

  /**
   * Reset the selection criteria for the chart.
   */
  bool ResetSelectionRange();

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

  ///@{
  /**
   * Specify a lookup table for the mapper to use.
   */
  void SetLookupTable(vtkScalarsToColors* lut);
  vtkScalarsToColors* GetLookupTable();
  ///@}

  /**
   * Create default lookup table. Generally used to create one when none
   * is available with the scalar data.
   */
  virtual void CreateDefaultLookupTable();

  ///@{
  /**
   * Turn on/off flag to control whether scalar data is used to color objects.
   */
  vtkSetMacro(ScalarVisibility, vtkTypeBool);
  vtkGetMacro(ScalarVisibility, vtkTypeBool);
  vtkBooleanMacro(ScalarVisibility, vtkTypeBool);
  ///@}

  ///@{
  /**
   * When ScalarMode is set to UsePointFieldData or UseCellFieldData,
   * you can specify which array to use for coloring using these methods.
   * The lookup table will decide how to convert vectors to colors.
   */
  void SelectColorArray(vtkIdType arrayNum);
  void SelectColorArray(const vtkStdString& arrayName);
  ///@}

  ///@{
  /**
   * Set/Get the color mode for the plot.
   *
   * The options are:
   * VTK_COLOR_MODE_DEFAULT
   * VTK_COLOR_MODE_MAP_SCALARS
   * VTK_COLOR_MODE_DIRECT_SCALARS
   *
   * Default is VTK_COLOR_MODE_MAP_SCALARS.
   */
  vtkSetMacro(ColorMode, int);
  void SetColorModeToDefault() { this->SetColorMode(VTK_COLOR_MODE_DEFAULT); }
  void SetColorModeToMapScalars() { this->SetColorMode(VTK_COLOR_MODE_MAP_SCALARS); }
  void SetColorModeToDirectScalars() { this->SetColorMode(VTK_COLOR_MODE_DIRECT_SCALARS); }
  vtkGetMacro(ColorMode, int);
  ///@}

  /**
   * Get the array name to color by.
   */
  vtkStdString GetColorArrayName();

  /**
   * Update the internal cache. Returns true if cache was successfully updated. Default does
   * nothing.
   * This method is called by Update() when either the plot's data has changed or
   * CacheRequiresUpdate() returns true. It is not necessary to call this method explicitly.
   */
  bool UpdateCache() override;

protected:
  vtkPlotParallelCoordinates();
  ~vtkPlotParallelCoordinates() override;

  ///@{
  /**
   * Store a well packed set of XY coordinates for this data series.
   */
  class Private;
  Private* Storage;
  ///@}

  ///@{
  /**
   * Lookup Table for coloring points by scalar value
   */
  vtkScalarsToColors* LookupTable;
  vtkUnsignedCharArray* Colors;
  vtkTypeBool ScalarVisibility;
  vtkStdString ColorArrayName;
  int ColorMode;
  ///@}

private:
  vtkPlotParallelCoordinates(const vtkPlotParallelCoordinates&) = delete;
  void operator=(const vtkPlotParallelCoordinates&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPlotParallelCoordinates_h
