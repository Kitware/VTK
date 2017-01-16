/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWidgetEvent.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class VTKINTERACTIONWIDGETS_EXPORT vtkWidgetEvent : public vtkObject
{
public:
  /**
   * The object factory constructor.
   */
  static vtkWidgetEvent *New() ;

  //@{
  /**
   * Standard macros.
   */
  vtkTypeMacro(vtkWidgetEvent,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * All the widget events are defined here.
   */
  enum WidgetEventIds {
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
    TimedOut,
    ModifyEvent,
    Reset,
    Up,
    Down,
    Left,
    Right
  };

  //@{
  /**
   * Convenience methods for translating between event names and event ids.
   */
  static const char *GetStringFromEventId(unsigned long event);
  static unsigned long GetEventIdFromString(const char *event);
  //@}

protected:
  vtkWidgetEvent() {}
  ~vtkWidgetEvent() VTK_OVERRIDE {}

private:
  vtkWidgetEvent(const vtkWidgetEvent&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWidgetEvent&) VTK_DELETE_FUNCTION;

};

#endif
