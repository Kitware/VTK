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
#include "vtkCommand.h"          // For vtkCommand enum
#include "vtkPlot.h"

class vtkColorTransferFunction;
class vtkBrush;

class VTKCHARTSCORE_EXPORT vtkRangeHandlesItem : public vtkPlot
{
public:
  vtkTypeMacro(vtkRangeHandlesItem, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkRangeHandlesItem* New();

  enum Handle
  {
    NO_HANDLE = -1,
    LEFT_HANDLE = 0,
    RIGHT_HANDLE = 1
  };

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

  //@{
  /**
   * Set/Get the handles width in pixels.
   * Default is 2.
   */
  vtkSetMacro(HandleWidth, float);
  vtkGetMacro(HandleWidth, float);
  //@}

  /**
   * Compute the handles draw range by using the handle width and the transfer function
   */
  void ComputeHandlesDrawRange();

protected:
  vtkRangeHandlesItem();
  ~vtkRangeHandlesItem() override;

  /**
   * Returns true if the supplied x, y coordinate is around a handle
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  //@{
  /**
   * Interaction methods to interact with the handles.
   */
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override;
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse) override;
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;
  bool MouseEnterEvent(const vtkContextMouseEvent& mouse) override;
  bool MouseLeaveEvent(const vtkContextMouseEvent& mouse) override;
  bool MouseDoubleClickEvent(const vtkContextMouseEvent& mouse) override;
  //@}

  /**
   * Returns the handle the provided point is over with a provided tolerance,
   * it can be NO_HANDLE, LEFT_HANDLE or RIGHT_HANDLE
   */
  virtual int FindRangeHandle(const vtkVector2f& point, const vtkVector2f& tolerance);

  /**
   * Internal method to set the ActiveHandlePosition
   * and compute the ActiveHandleRangeValue accordingly
   */
  void SetActiveHandlePosition(double position);

  /**
   * Internal method to check if the active handle have
   * actually been moved.
   */
  bool IsActiveHandleMoved(double tolerance);

  /**
   * Set the cursor shape
   */
  void SetCursor(int cursor);

private:
  vtkRangeHandlesItem(const vtkRangeHandlesItem&) = delete;
  void operator=(const vtkRangeHandlesItem&) = delete;

  vtkColorTransferFunction* ColorTransferFunction = nullptr;

  float HandleWidth = 2;
  float HandleDelta = 0;
  float LeftHandleDrawRange[2] = { 0, 0 };
  float RightHandleDrawRange[2] = { 0, 0 };
  int ActiveHandle = NO_HANDLE;
  int HoveredHandle = NO_HANDLE;
  double ActiveHandlePosition = 0;
  double ActiveHandleRangeValue = 0;
  vtkNew<vtkBrush> HighlightBrush;
  vtkNew<vtkBrush> RangeLabelBrush;
};

#endif // vtkRangeHandlesItem_h
