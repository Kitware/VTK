// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkColorTransferFunctionItem_h
#define vtkColorTransferFunctionItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkScalarsToColorsItem.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkColorTransferFunction;
class vtkImageData;

// Description:
// vtkPlot::Color, vtkPlot::Brush, vtkScalarsToColors::DrawPolyLine,
// vtkScalarsToColors::MaskAboveCurve have no effect here.
class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkColorTransferFunctionItem
  : public vtkScalarsToColorsItem
{
public:
  static vtkColorTransferFunctionItem* New();
  vtkTypeMacro(vtkColorTransferFunctionItem, vtkScalarsToColorsItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetColorTransferFunction(vtkColorTransferFunction* t);
  vtkGetObjectMacro(ColorTransferFunction, vtkColorTransferFunction);

protected:
  vtkColorTransferFunctionItem();
  ~vtkColorTransferFunctionItem() override;

  // Description:
  // Reimplemented to return the range of the lookup table
  void ComputeBounds(double bounds[4]) override;

  void ComputeTexture() override;
  vtkColorTransferFunction* ColorTransferFunction;

  /**
   * Override the histogram plotbar configuration
   * in order to set the color transfer function on it
   */
  bool ConfigurePlotBar() override;

private:
  vtkColorTransferFunctionItem(const vtkColorTransferFunctionItem&) = delete;
  void operator=(const vtkColorTransferFunctionItem&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
