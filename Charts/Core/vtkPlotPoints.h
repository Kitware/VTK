// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPlotPoints
 * @brief   Class for drawing an points given two columns from a
 * vtkTable.
 *
 *
 * This class draws points in a plot given two columns from a table. If you need
 * a line as well you should use vtkPlotLine which derives from vtkPlotPoints
 * and is capable of drawing both points and a line.
 *
 * @sa
 * vtkPlotLine
 */

#ifndef vtkPlotPoints_h
#define vtkPlotPoints_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkNew.h"              // For ivars
#include "vtkPlot.h"
#include "vtkRenderingCoreEnums.h" // For marker enum
#include "vtkScalarsToColors.h"    // For VTK_COLOR_MODE_DEFAULT and _MAP_SCALARS
#include "vtkStdString.h"          // For color array name
#include "vtkWrappingHints.h"      // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCharArray;
class vtkContext2D;
class vtkTable;
class vtkPoints2D;
class vtkFloatArray;
class vtkImageData;
class vtkScalarsToColors;
class vtkUnsignedCharArray;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPlotPoints : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotPoints, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a 2D Chart object.
   */
  static vtkPlotPoints* New();

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
   * Get the bounds for this plot as (Xmin, Xmax, Ymin, Ymax).
   */
  void GetBounds(double bounds[4]) override;

  /**
   * Get the non-log-scaled bounds on chart inputs for this plot as (Xmin, Xmax, Ymin, Ymax).
   */
  void GetUnscaledInputBounds(double bounds[4]) override;

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

  /**
   * Get the array name to color by.
   */
  vtkStdString GetColorArrayName();

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
   * Select all points in the specified polygon.
   */
  bool SelectPointsInPolygon(const vtkContextPolygon& polygon) override;

  /**
   * Enum containing various marker styles that can be used in a plot.
   */
  enum
  {
    NONE = VTK_MARKER_NONE,
    CROSS = VTK_MARKER_CROSS,
    PLUS = VTK_MARKER_PLUS,
    SQUARE = VTK_MARKER_SQUARE,
    CIRCLE = VTK_MARKER_CIRCLE,
    DIAMOND = VTK_MARKER_DIAMOND
  };

  ///@{
  /**
   * Get/set the marker style that should be used. The default is none, the enum
   * in this class is used as a parameter.
   */
  vtkGetMacro(MarkerStyle, int);
  vtkSetMacro(MarkerStyle, int);
  ///@}

  ///@{
  /**
   * Get/set the marker size that should be used. The default is negative, and
   * in that case it is 2.3 times the pen width, if less than 8 will be used.
   */
  vtkGetMacro(MarkerSize, float);
  vtkSetMacro(MarkerSize, float);
  ///@}

  ///@{
  /**
   * Get/set the valid point mask array name.
   */
  vtkGetMacro(ValidPointMaskName, vtkStdString);
  vtkSetMacro(ValidPointMaskName, vtkStdString);
  ///@}

  /**
   * Update the internal cache. Returns true if cache was successfully updated. Default does
   * nothing.
   * This method is called by Update() when either the plot's data has changed or
   * CacheRequiresUpdate() returns true. It is not necessary to call this method explicitly.
   */
  bool UpdateCache() override;

  void ReleaseGraphicsCache() override;

protected:
  vtkPlotPoints();
  ~vtkPlotPoints() override;

  /**
   * Populate the data arrays ready to operate on input data.
   */
  bool GetDataArrays(vtkTable* table, vtkDataArray* array[2]);

  /**
   * Test if the internal cache requires an update.
   */
  bool CacheRequiresUpdate() override;

  /**
   * Calculate the unscaled input bounds from the input arrays.
   */
  void CalculateUnscaledInputBounds();

  /**
   * Handle calculating the log of the x or y series if necessary. Should be
   * called by UpdateCache once the data has been updated in Points.
   */
  void CalculateLogSeries();

  /**
   * Find all of the "bad points" in the series. This is mainly used to cache
   * bad points for performance reasons, but could also be used plot the bad
   * points in the future.
   */
  void FindBadPoints();

  /**
   * Calculate the bounds of the plot, ignoring the bad points.
   */
  void CalculateBounds(double bounds[4]);

  /**
   * Create the sorted point list if necessary.
   */
  void CreateSortedPoints();

  ///@{
  /**
   * Store a well packed set of XY coordinates for this data series.
   */
  vtkPoints2D* Points;
  vtkNew<vtkFloatArray> SelectedPoints;
  ///@}

  ///@{
  /**
   * Sorted points, used when searching for the nearest point.
   */
  class VectorPIMPL;
  VectorPIMPL* Sorted;
  ///@}

  /**
   * An array containing the indices of all the "bad points", meaning any x, y
   * pair that has an infinity, -infinity or not a number value.
   */
  vtkIdTypeArray* BadPoints;

  /**
   * Array which marks valid points in the array. If nullptr (the default), all
   * points in the input array are considered valid.
   */
  vtkCharArray* ValidPointMask;

  /**
   * Name of the valid point mask array.
   */
  vtkStdString ValidPointMaskName;

  ///@{
  /**
   * The marker style that should be used
   */
  int MarkerStyle;
  float MarkerSize;
  ///@}

  bool LogX, LogY;

  ///@{
  /**
   * Lookup Table for coloring points by scalar value
   */
  vtkScalarsToColors* LookupTable;
  vtkUnsignedCharArray* Colors;
  vtkTypeBool ScalarVisibility;
  vtkStdString ColorArrayName;
  ///@}

  /**
   * Cached bounds on the plot input axes
   */
  double UnscaledInputBounds[4];

private:
  vtkPlotPoints(const vtkPlotPoints&) = delete;
  void operator=(const vtkPlotPoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPlotPoints_h
