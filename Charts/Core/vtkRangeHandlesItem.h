/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRangeHandlesItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkRangeHandlesItem
 * @brief   item to show and control the range of a vtkColorTransferFunction
 *
 * vtkRangeHandlesItem provides range handles painting and management
 * for a provided vtkColorTransferFunction.
 * Handles can be moved by clicking on them.
 * The range is shown when hovering or moving the handles.
 * It emits a StartInteractionEvent when starting to interact with a handle,
 * an InteractionEvent when interacting with a handle and an EndInteractionEvent
 * when releasing a handle.
 * It emits a LeftMouseButtonDoubleClickEvent when double clicked.
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

class vtkColorTransferFunction;

class VTKCHARTSCORE_EXPORT vtkRangeHandlesItem : public vtkPlotRangeHandlesItem
{
public:
  vtkTypeMacro(vtkRangeHandlesItem, vtkPlotRangeHandlesItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkRangeHandlesItem* New();

  /**
   * Paint both handles and the range if
   * a handle is active or hovered
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Recover the bounds of the item
   */
  void GetBounds(double bounds[4]) override;

  /**
   * Recover the range currently set by the handles
   * Use this method by observing EndInteractionEvent
   */
  virtual void GetHandlesRange(double range[2]);

  //@{
  /**
   * Get/set the color transfer function to interact with.
   */
  void SetColorTransferFunction(vtkColorTransferFunction* ctf);
  vtkGetObjectMacro(ColorTransferFunction, vtkColorTransferFunction);
  //@}

  /**
   * Compute the handles draw range by using the handle width and the transfer function
   */
  void ComputeHandlesDrawRange();

  void SynchronizeRangeHandlesOn() override;

  void SetSynchronizeHandlesRange(bool synchronize)
  {
    this->Superclass::SynchronizeRangeHandlesOff();
  }

  void SetHandleOrientation(int orientation)
  {
    this->Superclass::SetHandleOrientation(Orientation::VERTICAL);
  }

protected:
  vtkRangeHandlesItem();
  ~vtkRangeHandlesItem() override;

  /**
   * Internal method to set the ActiveHandlePosition
   * and compute the ActiveHandleRangeValue accordingly
   */
  void SetActiveHandlePosition(double position) override;

private:
  vtkRangeHandlesItem(const vtkRangeHandlesItem&) = delete;
  void operator=(const vtkRangeHandlesItem&) = delete;

  vtkColorTransferFunction* ColorTransferFunction = nullptr;
};

#endif // vtkRangeHandlesItem_h
