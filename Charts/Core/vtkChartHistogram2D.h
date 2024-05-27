// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkChart2DHistogram
 * @brief   Chart for 2D histograms.
 *
 *
 * This defines the interface for a 2D histogram chart.
 */

#ifndef vtkChartHistogram2D_h
#define vtkChartHistogram2D_h

#include "vtkChartXY.h"
#include "vtkChartsCoreModule.h" // For export macro
#include "vtkSmartPointer.h"     // For SP ivars
#include "vtkWrappingHints.h"    // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkColorLegend;
class vtkPlotHistogram2D;
class vtkImageData;
class vtkScalarsToColors;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkChartHistogram2D : public vtkChartXY
{
public:
  vtkTypeMacro(vtkChartHistogram2D, vtkChartXY);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a 2D histogram chart
   */
  static vtkChartHistogram2D* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  virtual void SetInputData(vtkImageData* data, vtkIdType z = 0);
  virtual void SetTransferFunction(vtkScalarsToColors* function);

  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  vtkPlot* GetPlot(vtkIdType index) override;

protected:
  vtkChartHistogram2D();
  ~vtkChartHistogram2D() override;

  vtkSmartPointer<vtkPlotHistogram2D> Histogram;

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  vtkTimeStamp BuildTime;

  class Private;
  Private* Storage;

  bool UpdateLayout(vtkContext2D* painter) override;

private:
  vtkChartHistogram2D(const vtkChartHistogram2D&) = delete;
  void operator=(const vtkChartHistogram2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkChartHistogram2D_h
