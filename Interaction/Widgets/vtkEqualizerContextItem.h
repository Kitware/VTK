// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkEqualizerContextItem_h
#define vtkEqualizerContextItem_h

#include "vtkContextItem.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"                      // For vtkNew

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkBrush;
class vtkPen;
class vtkContextTransform;

/**
 * @class vtkEqualizerContextItem
 *
 * @brief draws a interactive polyline
 *
 * This is a vtkContextItem that can be placed into a vtkContextScene.
 * It draws a polyline, and reacts to mouse events.
 * Initially there are 2 points at the ends of the line.
 * Provides the ability to add, remove, and move anchor points.
 * This is not a universal polyline and is designed to adjust the reference points for the digital
 * signal processing algorithm. So there are a number of features for interacting with the line:
 * - the horizontal axis is frequencies; the values are of the integer type and cannot be negative
 * - the vertical axis is the gains; the values are of the double type and cannot be negative
 * - each reference point is limited by the values of neighboring points on the x-axis
 */

class VTKINTERACTIONWIDGETS_EXPORT vtkEqualizerContextItem : public vtkContextItem
{
public:
  static vtkEqualizerContextItem* New();

  vtkTypeMacro(vtkEqualizerContextItem, vtkContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Paint event for the item, called whenever the item needs to be drawn.
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse enter event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  bool MouseEnterEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse leave event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  bool MouseLeaveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button down event
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button release event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse) override;

  ///@{
  /**
   * Set / Get anchor points in the following format
   * "P1x,P1y;P2x,P2y; ... PNx,PNy;"
   * where X denotes the frequency, typeid(x) = int
   * and Y denotes the gain, typeid(y) = float
   * "0,1;500,1;" by default
   */
  void SetPoints(const std::string& points);
  std::string GetPoints() const;
  ///@}

protected:
  enum MouseStates
  {
    NO_BUTTON = 0,
    LEFT_BUTTON_PRESSED = 1,
    RIGHT_BUTTON_PRESSED = 2
  };

  vtkEqualizerContextItem();
  ~vtkEqualizerContextItem() override;

  MouseStates MouseState = NO_BUTTON;
  vtkNew<vtkPen> Pen;
  vtkNew<vtkBrush> Brush;

private:
  vtkEqualizerContextItem(const vtkEqualizerContextItem&) = delete;
  void operator=(const vtkEqualizerContextItem&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
