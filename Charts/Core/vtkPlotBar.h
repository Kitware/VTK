// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPlotBar
 * @brief   Class for drawing an XY plot given two columns from a
 * vtkTable.
 *
 *
 *
 */

#ifndef vtkPlotBar_h
#define vtkPlotBar_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlot.h"
#include "vtkSmartPointer.h"  // Needed to hold ColorSeries
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkContext2D;
class vtkTable;
class vtkPoints2D;
class vtkStdString;
class vtkColorSeries;
class vtkUnsignedCharArray;
class vtkScalarsToColors;

class vtkPlotBarPrivate;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPlotBar : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotBar, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Enum of bar chart oritentation types
   */
  enum
  {
    VERTICAL = 0,
    HORIZONTAL
  };

  /**
   * Creates a 2D Chart object.
   */
  static vtkPlotBar* New();

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

  ///@{
  /**
   * Set the width of the line.
   */
  void SetWidth(float _arg) override
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Width to " << _arg);
    if (this->Width != _arg)
    {
      this->Width = _arg;
      this->Modified();
    }
  }
  ///@}

  ///@{
  /**
   * Get the width of the line.
   */
  float GetWidth() override
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning Width of "
                  << this->Width);
    return this->Width;
  }
  ///@}

  ///@{
  /**
   * Set/get the horizontal offset of the bars.
   * Positive values move the bars leftward.
   * For HORIZONTAL orientation, offsets bars vertically,
   * with a positive value moving bars downward.
   */
  vtkSetMacro(Offset, float);
  vtkGetMacro(Offset, float);
  ///@}

  ///@{
  /**
   * Set/get the orientation of the bars.
   * Valid orientations are VERTICAL (default) and HORIZONTAL.
   */
  virtual void SetOrientation(int orientation);
  vtkGetMacro(Orientation, int);
  ///@}

  /**
   * A helper used by both GetUnscaledBounds and GetBounds(double[4]).
   */
  virtual void GetBounds(double bounds[4], bool unscaled);

  /**
   * Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax).
   */
  void GetBounds(double bounds[4]) override;

  /**
   * Get un-log-scaled bounds for this mapper as (Xmin,Xmax,Ymin,Ymax).
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

  ///@{
  /**
   * Specify a lookup table for the mapper to use.
   */
  virtual void SetLookupTable(vtkScalarsToColors* lut);
  virtual vtkScalarsToColors* GetLookupTable();
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
  vtkSetMacro(ScalarVisibility, bool);
  vtkGetMacro(ScalarVisibility, bool);
  vtkBooleanMacro(ScalarVisibility, bool);
  ///@}

  ///@{
  /**
   * Enable/disable mapping of the opacity values. Default is set to true.
   */
  vtkSetMacro(EnableOpacityMapping, bool);
  vtkGetMacro(EnableOpacityMapping, bool);
  vtkBooleanMacro(EnableOpacityMapping, bool);
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
   * Get the plot labels.
   */
  vtkStringArray* GetLabels() override;

  /**
   * Set the group name of the bar chart - can be displayed on the X axis.
   */
  virtual void SetGroupName(const vtkStdString& name);

  /**
   * Get the group name of the bar char - can be displayed on the X axis.
   */
  virtual vtkStdString GetGroupName();

  /**
   * Generate and return the tooltip label string for this plot
   * The segmentIndex is implemented here.
   */
  vtkStdString GetTooltipLabel(
    const vtkVector2d& plotPos, vtkIdType seriesIndex, vtkIdType segmentIndex) override;

  /**
   * Select all points in the specified rectangle.
   */
  bool SelectPoints(const vtkVector2f& min, const vtkVector2f& max) override;

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated or
   * -1.
   * If a vtkIdType* is passed, its referent will be set to index of the bar
   * segment with which a point is associated, or -1.
   */
  vtkIdType GetNearestPoint(const vtkVector2f& point, const vtkVector2f&, vtkVector2f* location,
    vtkIdType* segmentIndex) override;
  using vtkPlot::GetNearestPoint;

  /**
   * Get amount of plotted bars.
   */
  int GetBarsCount();

  /**
   * Get the data bounds for this mapper as (Xmin,Xmax).
   */
  void GetDataBounds(double bounds[2]);

  /**
   * Update the internal cache. Returns true if cache was successfully updated. Default does
   * nothing.
   * This method is called by Update() when either the plot's data has changed or
   * CacheRequiresUpdate() returns true. It is not necessary to call this method explicitly.
   */
  bool UpdateCache() override;

protected:
  vtkPlotBar();
  ~vtkPlotBar() override;

  /**
   * Test if the internal cache requires an update.
   */
  bool CacheRequiresUpdate() override;

  /**
   * Store a well packed set of XY coordinates for this data series.
   */
  vtkPoints2D* Points;

  float Width;
  float Offset;

  int Orientation;

  /**
   * The color series to use if this becomes a stacked bar
   */
  vtkSmartPointer<vtkColorSeries> ColorSeries;

  ///@{
  /**
   * Lookup Table for coloring bars by scalar value
   */
  vtkSmartPointer<vtkScalarsToColors> LookupTable;
  vtkSmartPointer<vtkUnsignedCharArray> Colors;
  bool ScalarVisibility;
  bool EnableOpacityMapping;
  vtkStdString ColorArrayName;
  ///@}

  bool LogX;
  bool LogY;

private:
  vtkPlotBar(const vtkPlotBar&) = delete;
  void operator=(const vtkPlotBar&) = delete;

  vtkPlotBarPrivate* Private;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPlotBar_h
