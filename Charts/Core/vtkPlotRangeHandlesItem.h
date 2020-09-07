/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotRangeHandlesItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkPlotRangeHandlesItem
 * @brief   item to show and control the range
 *
 * vtkPlotRangeHandlesItem provides range handles painting and management
 * for a provided extent.
 * Handles can be moved by clicking on them.
 * The range is shown when hovering or moving the handles.
 * It emits a StartInteractionEvent when starting to interact with a handle,
 * an InteractionEvent when interacting with a handle and an EndInteractionEvent
 * when releasing a handle.
 *
 */

#ifndef vtkPlotRangeHandlesItem_h
#define vtkPlotRangeHandlesItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkCommand.h"          // For vtkCommand enum
#include "vtkPlot.h"

class vtkColorTransferFunction;
class vtkBrush;

class VTKCHARTSCORE_EXPORT vtkPlotRangeHandlesItem : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotRangeHandlesItem, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPlotRangeHandlesItem* New();

  enum Handle
  {
    NO_HANDLE = -1,
    LEFT_HANDLE = 0,
    RIGHT_HANDLE = 1
  };

  enum Orientation
  {
    VERTICAL = 0,
    HORIZONTAL = 1
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
   * Set/Get the handles width in pixels.
   * Default is 2.
   */
  vtkSetMacro(HandleWidth, float);
  vtkGetMacro(HandleWidth, float);
  //@}

  //@{
  /**
   * Set/Get the handles orientation in the plot.
   */
  vtkSetClampMacro(HandleOrientation, int, VERTICAL, HORIZONTAL);
  vtkGetMacro(HandleOrientation, int);
  void SetHandleOrientationToVertical() { this->SetHandleOrientation(VERTICAL); }
  void SetHandleOrientationToHorizontal() { this->SetHandleOrientation(HORIZONTAL); }
  //@}

  //@{
  /**
   * Set/Get the extent of the handles in data space (axis unscaled range).
   * The first two parameters define the left and right handles positions on
   * the axis. The last two parameters define the length of handles along the
   * opposite axis. Default values are set to (0, 1, 0, 1).
   * When using ExtentToAxisRangeOn(), the last two parameters don't have any
   * effect and handles span the axis range.
   */
  vtkSetVector4Macro(Extent, double);
  vtkGetVector4Macro(Extent, double);
  //@}

  //@{
  /**
   * Set/Get whether handles span the range of the axis. Default is On.
   */
  vtkSetMacro(ExtentToAxisRange, vtkTypeBool);
  vtkGetMacro(ExtentToAxisRange, vtkTypeBool);
  vtkBooleanMacro(ExtentToAxisRange, vtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get whether handles move together when one of them is update. Default is Off.
   */
  vtkSetMacro(SynchronizeRangeHandles, vtkTypeBool);
  vtkGetMacro(SynchronizeRangeHandles, vtkTypeBool);
  vtkBooleanMacro(SynchronizeRangeHandles, vtkTypeBool);
  //@}

  /**
   * Compute the handles draw range by using the handle width and the transfer function
   */
  void ComputeHandlesDrawRange();

protected:
  vtkPlotRangeHandlesItem();
  ~vtkPlotRangeHandlesItem() override;

  //@{
  /**
   * Get the logical range of abcissa or ordinate axis based on the handle
   * orientation, in plot coordinates.

   * The unscaled range will always be in the same coordinate system of
   * the data being plotted, regardless of whether LogScale is true or false.
   * Calling GetAxisRange() when LogScale is true will return the log10({min, max}).
   */
  void GetAxesRange(double* abcissaRange, double* ordinateRange);
  void GetAxesUnscaledRange(double* abcissaRange, double* ordinateRange);
  //@}

  /**
   * Compute the range used for the handles.
   */
  void ComputeRange(double* range);

  /**
   * Compute the delta used for the picking handle size.
   */
  void ComputeHandleDelta(double screenBounds[4]);

  //@{
  /**
   * Transform the mouse event in the control-points space. This is needed when
   * using logScale or shiftscale.
   */
  void TransformScreenToData(
    const double inX, const double inY, double& outX, double& outY) override;
  void TransformDataToScreen(
    const double inX, const double inY, double& outX, double& outY) override;
  //@}

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
  virtual void SetActiveHandlePosition(double position);

  /**
   * Internal method to check if the active handle have
   * actually been moved.
   */
  bool IsActiveHandleMoved(double tolerance);

  /**
   * Set the cursor shape
   */
  void SetCursor(int cursor);

  vtkPlotRangeHandlesItem(const vtkPlotRangeHandlesItem&) = delete;
  void operator=(const vtkPlotRangeHandlesItem&) = delete;

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
  double Extent[4] = { 0, 0, 0, 0 };
  vtkTypeBool ExtentToAxisRange = true;
  vtkTypeBool SynchronizeRangeHandles = false;
  int HandleOrientation = VERTICAL;
};

#endif // vtkPlotRangeHandlesItem_h
