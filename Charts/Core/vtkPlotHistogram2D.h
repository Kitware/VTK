// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtk2DHistogramItem
 * @brief   2D histogram item.
 *
 *
 *
 */

#ifndef vtkPlotHistogram2D_h
#define vtkPlotHistogram2D_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkNew.h"              // For vtkNew
#include "vtkPlot.h"
#include "vtkRect.h"          // Needed for vtkRectf
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <string> // Needed for std::string

VTK_ABI_NAMESPACE_BEGIN

class vtkDoubleArray;
class vtkImageData;
class vtkScalarsToColors;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPlotHistogram2D : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotHistogram2D, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a new object.
   */
  static vtkPlotHistogram2D* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the item, called whenever it needs to be drawn.
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Set the input. The image data is supposed to have scalars attribute set,
   * if no array name is set.
   */
  virtual void SetInputData(vtkImageData* data, vtkIdType z = 0);
  void SetInputData(vtkTable*) override {}
  void SetInputData(vtkTable*, const vtkStdString&, const vtkStdString&) override {}

  /**
   * Get the input table used by the plot.
   */
  vtkImageData* GetInputImageData();

  /**
   * Set the color transfer function that will be used to generate the 2D
   * histogram.
   */
  void SetTransferFunction(vtkScalarsToColors* transfer);

  /**
   * Get the color transfer function that is used to generate the histogram.
   */
  vtkScalarsToColors* GetTransferFunction();

  void GetBounds(double bounds[4]) override;

  virtual void SetPosition(const vtkRectf& pos);
  virtual vtkRectf GetPosition();

  ///@{
  /**
   * Set/get the selected array name.
   * When empty, plot using SCALARS attribute.
   * Default: empty string (use SCALARS).
   */
  vtkSetMacro(ArrayName, std::string);
  vtkGetMacro(ArrayName, std::string);
  ///@}

  /**
   * Generate and return the tooltip label string for this plot
   * The segmentIndex parameter is ignored.
   * The member variable TooltipLabelFormat can be set as a
   * printf-style string to build custom tooltip labels from,
   * and may contain:
   * An empty string generates the default tooltip labels.
   * The following case-sensitive format tags (without quotes) are recognized:
   * '%x' The X position of the histogram cell
   * '%y' The Y position of the histogram cell
   * '%v' The scalar value of the histogram cell
   * '%i' The X axis tick label for the histogram cell
   * '%j' The Y axis tick label for the histogram cell
   * Any other characters or unrecognized format tags are printed in the
   * tooltip label verbatim.
   */
  vtkStdString GetTooltipLabel(
    const vtkVector2d& plotPos, vtkIdType seriesIndex, vtkIdType segmentIndex) override;

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns an index between 0 and (number of histogram cells - 1), or -1.
   * The index 0 is at cell x=0, y=0 of the histogram, and the index increases
   * in a minor fashion with x and in a major fashion with y.
   * The referent of "location" is set to the x and y integer indices of the
   * histogram cell.
   */
  vtkIdType GetNearestPoint(const vtkVector2f& point, const vtkVector2f& tolerance,
    vtkVector2f* location, vtkIdType* segmentId) override;
  using vtkPlot::GetNearestPoint;

  /**
   * Update the internal cache. Returns true if cache was successfully updated. Default does
   * nothing.
   * This method is called by Update() when either the plot's data has changed or
   * CacheRequiresUpdate() returns true. It is not necessary to call this method explicitly.
   */
  bool UpdateCache() override;

protected:
  vtkPlotHistogram2D();
  ~vtkPlotHistogram2D() override;

  vtkSmartPointer<vtkImageData> Input;
  vtkSmartPointer<vtkImageData> Output;
  vtkSmartPointer<vtkScalarsToColors> TransferFunction;
  vtkRectf Position;

private:
  vtkPlotHistogram2D(const vtkPlotHistogram2D&) = delete;
  void operator=(const vtkPlotHistogram2D&) = delete;

  /**
   * Returns the index of the label of an axis, depending on a
   * position on the axis.
   */
  static vtkIdType GetLabelIndexFromValue(double value, vtkAxis* axis);
  /**
   * Returns whether an array is compatible with magnitude computation,
   * ie. its number of component is 2 or 3.
   */
  static inline bool CanComputeMagnitude(vtkDataArray* array);

  /**
   * Returns the selected data array. Does not return magnitude
   * array, but the associated array of the input.
   */
  inline vtkDataArray* GetSelectedArray();
  /**
   * Returns the void pointer to the selected array. If the transfer
   * function is set to magnitude mode, it will return the cached
   * magnitude array. Also set the vtkDataArray pointer in parameter.
   */
  void* GetInputArrayPointer(vtkDataArray*& inputArray);
  /**
   * Returns the value of the selected array at the coordinates given
   * in parameters. The value is casted to double. It takes magnitude
   * array into account, so as component, for n-components arrays.
   * Returns NaN when something goes wrong.
   */
  double GetInputArrayValue(int x, int y, int z);

  std::string ArrayName;
  vtkNew<vtkDoubleArray> MagnitudeArray;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPlotHistogram2D_h
