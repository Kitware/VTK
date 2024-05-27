// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkScalarsToColorsItem
 * @brief   Abstract class for ScalarsToColors items.
 *
 * vtkScalarsToColorsItem implements item bounds and painting for inherited
 * classes that provide a texture (ComputeTexture()) and optionally a shape
 * @sa
 * vtkControlPointsItem
 * vtkLookupTableItem
 * vtkColorTransferFunctionItem
 * vtkCompositeTransferFunctionItem
 * vtkPiecewiseItemFunctionItem
 */

#ifndef vtkScalarsToColorsItem_h
#define vtkScalarsToColorsItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkNew.h"              // For vtkNew
#include "vtkPlot.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCallbackCommand;
class vtkImageData;
class vtkPlotBar;
class vtkPoints2D;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkScalarsToColorsItem : public vtkPlot
{
public:
  vtkTypeMacro(vtkScalarsToColorsItem, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Bounds of the item, use the UserBounds if valid otherwise compute
   * the bounds of the item (based on the transfer function range).
   */
  void GetBounds(double bounds[4]) override;

  ///@{
  /**
   * Set custom bounds, except if bounds are invalid, bounds will be
   * automatically computed based on the range of the control points
   * Invalid bounds by default.
   */
  vtkSetVector4Macro(UserBounds, double);
  vtkGetVector4Macro(UserBounds, double);
  ///@}

  /**
   * Paint the texture into a rectangle defined by the bounds. If
   * MaskAboveCurve is true and a shape has been provided by a subclass, it
   * draws the texture into the shape
   */
  bool Paint(vtkContext2D* painter) override;

  ///@{
  /**
   * Get a pointer to the vtkPen object that controls the drawing of the edge
   * of the shape if any.
   * PolyLinePen type is vtkPen::NO_PEN by default.
   */
  vtkGetObjectMacro(PolyLinePen, vtkPen);
  ///@}

  ///@{
  /**
   * Set/Get the vtkTable displayed as an histogram using a vtkPlotBar
   */
  void SetHistogramTable(vtkTable* histogramTable);
  vtkGetObjectMacro(HistogramTable, vtkTable);
  ///@}

  ///@{
  /**
   * Don't fill in the part above the transfer function.
   * If true texture is not visible above the shape provided by subclasses,
   * otherwise the whole rectangle defined by the bounds is filled with the
   * transfer function.
   * Note: only 2D transfer functions (RGB tf + alpha tf ) support the feature.
   */
  vtkSetMacro(MaskAboveCurve, bool);
  vtkGetMacro(MaskAboveCurve, bool);
  ///@}

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
   * Generate and return the tooltip label string for this plot
   * The segmentIndex is implemented here.
   */
  vtkStdString GetTooltipLabel(
    const vtkVector2d& plotPos, vtkIdType seriesIndex, vtkIdType segmentIndex) override;

protected:
  vtkScalarsToColorsItem();
  ~vtkScalarsToColorsItem() override;

  /**
   * Bounds of the item, by default (0, 1, 0, 1) but it depends on the
   * range of the ScalarsToColors function.
   * Need to be reimplemented by subclasses if the range is != [0,1]
   */
  virtual void ComputeBounds(double* bounds);

  /**
   * Need to be reimplemented by subclasses, ComputeTexture() is called at
   * paint time if the texture is not up to date compared to vtkScalarsToColorsItem
   * Return false if no texture is generated.
   */
  virtual void ComputeTexture() = 0;

  vtkGetMacro(TextureWidth, int);

  /**
   * Method to configure the plotbar histogram before painting it
   * can be reimplemented by subclasses.
   * Return true if the histogram should be painted, false otherwise.
   */
  virtual bool ConfigurePlotBar();

  ///@{
  /**
   * Called whenever the ScalarsToColors function(s) is modified. It internally
   * calls Modified(). Can be reimplemented by subclasses
   */
  virtual void ScalarsToColorsModified(vtkObject* caller, unsigned long eid, void* calldata);
  static void OnScalarsToColorsModified(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  ///@}

  double UserBounds[4];

  bool Interpolate = true;
  int TextureWidth;
  vtkImageData* Texture = nullptr;
  vtkTable* HistogramTable = nullptr;

  vtkNew<vtkPoints2D> Shape;
  vtkNew<vtkCallbackCommand> Callback;
  vtkNew<vtkPlotBar> PlotBar;
  vtkNew<vtkPen> PolyLinePen;
  bool MaskAboveCurve;

private:
  vtkScalarsToColorsItem(const vtkScalarsToColorsItem&) = delete;
  void operator=(const vtkScalarsToColorsItem&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
