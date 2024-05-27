// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkPiecewiseFunctionItem_h
#define vtkPiecewiseFunctionItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkScalarsToColorsItem.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkPiecewiseFunction;
class vtkImageData;

/// vtkPiecewiseFunctionItem internal uses vtkPlot::Color, white by default
class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPiecewiseFunctionItem : public vtkScalarsToColorsItem
{
public:
  static vtkPiecewiseFunctionItem* New();
  vtkTypeMacro(vtkPiecewiseFunctionItem, vtkScalarsToColorsItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetPiecewiseFunction(vtkPiecewiseFunction* t);
  vtkGetObjectMacro(PiecewiseFunction, vtkPiecewiseFunction);

protected:
  vtkPiecewiseFunctionItem();
  ~vtkPiecewiseFunctionItem() override;

  // Description:
  // Reimplemented to return the range of the piecewise function
  void ComputeBounds(double bounds[4]) override;

  // Description
  // Compute the texture from the PiecewiseFunction
  void ComputeTexture() override;

  vtkPiecewiseFunction* PiecewiseFunction;

private:
  vtkPiecewiseFunctionItem(const vtkPiecewiseFunctionItem&) = delete;
  void operator=(const vtkPiecewiseFunctionItem&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
