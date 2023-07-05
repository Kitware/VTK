// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWidgetEvent
 * @brief   define widget events
 *
 * vtkWidgetEvent defines widget events. These events are processed by
 * subclasses of vtkInteractorObserver.
 */

#ifndef vtkWidgetEvent_h
#define vtkWidgetEvent_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONWIDGETS_EXPORT vtkWidgetEvent : public vtkObject
{
public:
  /**
   * The object factory constructor.
   */
  static vtkWidgetEvent* New();

  ///@{
  /**
   * Standard macros.
   */
  vtkTypeMacro(vtkWidgetEvent, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * All the widget events are defined here.
   */
  enum WidgetEventIds
  {
    NoEvent = 0,
    Select,
    EndSelect,
    Delete,
    Translate,
    EndTranslate,
    Scale,
    EndScale,
    Resize,
    EndResize,
    Rotate,
    EndRotate,
    Move,
    SizeHandles,
    AddPoint,
    AddFinalPoint,
    Completed,
    PickPoint,
    PickNormal,
    PickDirectionPoint,
    TimedOut,
    ModifyEvent,
    Reset,
    Up,
    Down,
    Left,
    Right,
    Select3D,
    EndSelect3D,
    Move3D,
    AddPoint3D,
    AddFinalPoint3D,
    HoverLeave
  };

  ///@{
  /**
   * Convenience methods for translating between event names and event ids.
   */
  static const char* GetStringFromEventId(unsigned long event);
  static unsigned long GetEventIdFromString(const char* event);
  ///@}

protected:
  vtkWidgetEvent() = default;
  ~vtkWidgetEvent() override = default;

private:
  vtkWidgetEvent(const vtkWidgetEvent&) = delete;
  void operator=(const vtkWidgetEvent&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
