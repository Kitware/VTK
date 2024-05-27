// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkRangeHandlesItem
 * @brief   item to show and control the range of a vtkColorTransferFunction
 *
 * vtkRangeHandlesItem provides range handles painting and management
 * for a provided vtkColorTransferFunction.
 * This specialization of vtkPlotRangeHandlesItem works in coordination with
 * the color transfer function to drive the behavior of handles.
 * Handles can only be dragged within the color transfer function range and
 * are forced to be placed vertically with a fixed height of 1.
 *
 * A typical use case for this class is to observe EndInteractionEvent to
 * update the color transfer function range using the handles range.
 *
 * @sa
 * vtkControlPointsItem
 * vtkScalarsToColorsItem
 * vtkColorTransferFunctionItem
 */

#ifndef vtkRangeHandlesItem_h
#define vtkRangeHandlesItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlotRangeHandlesItem.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkColorTransferFunction;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkRangeHandlesItem : public vtkPlotRangeHandlesItem
{
public:
  vtkTypeMacro(vtkRangeHandlesItem, vtkPlotRangeHandlesItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkRangeHandlesItem* New();

  /**
   * Overridden to check that a color transfer function has been set before
   * painting.
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Overridden to get the bounds from the color transfer function range.
   */
  void GetBounds(double bounds[4]) override;

  /**
   * Overridden to return the range of the color transfer function.
   * Use this method by observing EndInteractionEvent
   */
  void GetHandlesRange(double range[2]) override;

  ///@{
  /**
   * Get/set the color transfer function to interact with.
   */
  void SetColorTransferFunction(vtkColorTransferFunction* ctf);
  vtkGetObjectMacro(ColorTransferFunction, vtkColorTransferFunction);
  ///@}

  /**
   * Compute the handles draw range by using the handle width and the transfer
   * function.
   */
  void ComputeHandlesDrawRange() override;

  ///@{
  /**
   * Overridden to force using desynchronized vertical handles.
   * Desynchronized handles means that handles are always moved independently,
   * as opposed to synchronized handles where the left handle drives the
   * modification of the whole range. See superclass for more information.
   */
  void SynchronizeRangeHandlesOn() override { this->Superclass::SynchronizeRangeHandlesOff(); }

  void SetSynchronizeRangeHandles(vtkTypeBool vtkNotUsed(synchronize)) override
  {
    this->Superclass::SynchronizeRangeHandlesOff();
  }

  void SetHandleOrientation(int vtkNotUsed(orientation)) override
  {
    this->Superclass::SetHandleOrientation(Orientation::VERTICAL);
  }
  ///@}

protected:
  vtkRangeHandlesItem();
  ~vtkRangeHandlesItem() override;

  /**
   * Overridden to clamp the handle position in the color transfer function
   * range.
   */
  void SetActiveHandlePosition(double position) override;

private:
  vtkRangeHandlesItem(const vtkRangeHandlesItem&) = delete;
  void operator=(const vtkRangeHandlesItem&) = delete;

  vtkColorTransferFunction* ColorTransferFunction = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif // vtkRangeHandlesItem_h
