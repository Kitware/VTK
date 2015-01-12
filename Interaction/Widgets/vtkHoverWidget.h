/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHoverWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHoverWidget - invoke a vtkTimerEvent when hovering
// .SECTION Description
// The vtkHoverWidget is used to invoke an event when hovering in a render window.
// Hovering occurs when mouse motion (in the render window) does not occur
// for a specified amount of time (i.e., TimerDuration). This class can be used
// as is (by observing TimerEvents) or for class derivation for those classes
// wishing to do more with the hover event.
//
// To use this widget, specify an instance of vtkHoverWidget and specify the
// time (in milliseconds) defining the hover period. Unlike most widgets,
// this widget does not require a representation (although subclasses like
// vtkBalloonWidget do require a representation).
//
// .SECTION Event Bindings
// By default, the widget observes the following VTK events (i.e., it
// watches the vtkRenderWindowInteractor for these events):
// <pre>
//   MouseMoveEvent - manages a timer used to determine whether the mouse
//                    is hovering.
//   TimerEvent - when the time between events (e.g., mouse move), then a
//                timer event is invoked.
//   KeyPressEvent - when the "Enter" key is pressed after the balloon appears,
//                   a callback is activated (e.g., WidgetActivateEvent).
// </pre>
//
// Note that the event bindings described above can be changed using this
// class's vtkWidgetEventTranslator. This class translates VTK events
// into the vtkHoverWidget's widget events:
// <pre>
//   vtkWidgetEvent::Move -- start (or reset) the timer
//   vtkWidgetEvent::TimedOut -- when enough time is elapsed between defined
//                               VTK events the hover event is invoked.
//   vtkWidgetEvent::SelectAction -- activate any callbacks associated
//                                   with the balloon.
// </pre>
//
// This widget invokes the following VTK events on itself when the widget
// determines that it is hovering. Note that observers of this widget can
// listen for these events and take appropriate action.
// <pre>
//   vtkCommand::TimerEvent (when hovering is determined to occur)
//   vtkCommand::EndInteractionEvent (after a hover has occurred and the
//                                    mouse begins moving again).
//   vtkCommand::WidgetActivateEvent (when the balloon is selected with a
//                                    keypress).
// </pre>

// .SECTION See Also
// vtkAbstractWidget


#ifndef vtkHoverWidget_h
#define vtkHoverWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"


class VTKINTERACTIONWIDGETS_EXPORT vtkHoverWidget : public vtkAbstractWidget
{
public:
  // Description:
  // Instantiate this class.
  static vtkHoverWidget *New();

  // Description:
  // Standard methods for a VTK class.
  vtkTypeMacro(vtkHoverWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the hovering interval (in milliseconds). If after moving the
  // mouse the pointer stays over a vtkProp for this duration, then a
  // vtkTimerEvent::TimerEvent is invoked.
  vtkSetClampMacro(TimerDuration,int,1,100000);
  vtkGetMacro(TimerDuration,int);

  // Description:
  // The method for activating and deactivating this widget. This method
  // must be overridden because it performs special timer-related operations.
  virtual void SetEnabled(int);

  // Description:
  // A default representation, of which there is none, is created. Note
  // that the superclasses vtkAbstractWidget::GetRepresentation()
  // method returns NULL.
  void CreateDefaultRepresentation()
    {this->WidgetRep = NULL;}

protected:
  vtkHoverWidget();
  ~vtkHoverWidget();

  // The state of the widget
//BTX
  enum {Start=0,Timing,TimedOut};
//ETX
  int WidgetState;

  // Callback interface to execute events
  static void MoveAction(vtkAbstractWidget*);
  static void HoverAction(vtkAbstractWidget*);
  static void SelectAction(vtkAbstractWidget*);

  // Subclasses of this class invoke these methods. If a non-zero
  // value is returned, a subclass is handling the event.
  virtual int SubclassHoverAction() {return 0;}
  virtual int SubclassEndHoverAction() {return 0;}
  virtual int SubclassSelectAction() {return 0;}

  // Description:
  // Helper methods for creating and destroying timers.
  int TimerId;
  int TimerDuration;

private:
  vtkHoverWidget(const vtkHoverWidget&);  //Not implemented
  void operator=(const vtkHoverWidget&);  //Not implemented
};

#endif
