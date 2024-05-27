// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkCompositeTransferFunctionItem_h
#define vtkCompositeTransferFunctionItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkColorTransferFunctionItem.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkPiecewiseFunction;

// Description:
// vtkPlot::Color and vtkPlot::Brush have no effect here.
class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkCompositeTransferFunctionItem
  : public vtkColorTransferFunctionItem
{
public:
  static vtkCompositeTransferFunctionItem* New();
  vtkTypeMacro(vtkCompositeTransferFunctionItem, vtkColorTransferFunctionItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetOpacityFunction(vtkPiecewiseFunction* opacity);
  vtkGetObjectMacro(OpacityFunction, vtkPiecewiseFunction);

protected:
  vtkCompositeTransferFunctionItem();
  ~vtkCompositeTransferFunctionItem() override;

  // Description:
  // Reimplemented to return the range of the piecewise function
  void ComputeBounds(double bounds[4]) override;

  void ComputeTexture() override;
  vtkPiecewiseFunction* OpacityFunction;

private:
  vtkCompositeTransferFunctionItem(const vtkCompositeTransferFunctionItem&) = delete;
  void operator=(const vtkCompositeTransferFunctionItem&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
