// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPiecewiseControlPointsItem
 * @brief   Control points for
 * vtkPiecewiseFunction.
 *
 * vtkPiecewiseControlPointsItem draws the control points of a vtkPiecewiseFunction.
 * @sa
 * vtkControlPointsItem
 * vtkPiecewiseFunctionItem
 * vtkCompositeTransferFunctionItem
 */

#ifndef vtkPiecewiseControlPointsItem_h
#define vtkPiecewiseControlPointsItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkControlPointsItem.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkPiecewiseFunction;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPiecewiseControlPointsItem
  : public vtkControlPointsItem
{
public:
  vtkTypeMacro(vtkPiecewiseControlPointsItem, vtkControlPointsItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a piecewise control points object
   */
  static vtkPiecewiseControlPointsItem* New();

  /**
   * Set the piecewise function to draw its points
   */
  virtual void SetPiecewiseFunction(vtkPiecewiseFunction* function);
  ///@{
  /**
   * Get the piecewise function
   */
  vtkGetObjectMacro(PiecewiseFunction, vtkPiecewiseFunction);
  ///@}

  /**
   * Add a point to the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  vtkIdType AddPoint(double* newPos) override;

  /**
   * Remove a point of the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  vtkIdType RemovePoint(double* pos) override;

protected:
  vtkPiecewiseControlPointsItem();
  ~vtkPiecewiseControlPointsItem() override;

  void emitEvent(unsigned long event, void* params = nullptr) override;

  vtkMTimeType GetControlPointsMTime() override;

  vtkIdType GetNumberOfPoints() const override;
  void GetControlPoint(vtkIdType index, double* point) const override;
  void SetControlPoint(vtkIdType index, double* point) override;
  void EditPoint(float tX, float tY) override;

  vtkPiecewiseFunction* PiecewiseFunction;

private:
  vtkPiecewiseControlPointsItem(const vtkPiecewiseControlPointsItem&) = delete;
  void operator=(const vtkPiecewiseControlPointsItem&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
