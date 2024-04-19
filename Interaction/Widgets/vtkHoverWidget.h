// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHoverWidget
 * @brief   invoke a vtkTimerEvent when hovering
 *
 * The vtkHoverWidget is used to invoke an event when hovering in a render window.
 * Hovering occurs when mouse motion (in the render window) does not occur
 * for a specified amount of time (i.e., TimerDuration). This class can be used
 * as is (by observing TimerEvents) or for class derivation for those classes
 * wishing to do more with the hover event.
 *
 * To use this widget, specify an instance of vtkHoverWidget and specify the
 * time (in milliseconds) defining the hover period. Unlike most widgets,
 * this widget does not require a representation (although subclasses like
 * vtkBalloonWidget do require a representation).
 *
 * @par Event Bindings:
 * By default, the widget observes the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 *   MouseMoveEvent - manages a timer used to determine whether the mouse
 *                    is hovering.
 *   TimerEvent - when the time between events (e.g., mouse move), then a
 *                timer event is invoked.
 *   KeyPressEvent - when the "Enter" key is pressed after the balloon appears,
 *                   a callback is activated (e.g., WidgetActivateEvent).
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkHoverWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Move -- start (or reset) the timer
 *   vtkWidgetEvent::TimedOut -- when enough time is elapsed between defined
 *                               VTK events the hover event is invoked.
 *   vtkWidgetEvent::SelectAction -- activate any callbacks associated
 *                                   with the balloon.
 * </pre>
 *
 * @par Event Bindings:
 * This widget invokes the following VTK events on itself when the widget
 * determines that it is hovering. Note that observers of this widget can
 * listen for these events and take appropriate action.
 * <pre>
 *   vtkCommand::TimerEvent (when hovering is determined to occur)
 *   vtkCommand::EndInteractionEvent (after a hover has occurred and the
 *                                    mouse begins moving again).
 *   vtkCommand::WidgetActivateEvent (when the balloon is selected with a
 *                                    keypress).
 * </pre>
 *
 * @sa
 * vtkAbstractWidget
 */

#ifndef vtkHoverWidget_h
#define vtkHoverWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkHoverWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkHoverWidget* New();

  ///@{
  /**
   * Standard methods for a VTK class.
   */
  vtkTypeMacro(vtkHoverWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the hovering interval (in milliseconds). If after moving the
   * mouse the pointer stays over a vtkProp for this duration, then a
   * vtkTimerEvent::TimerEvent is invoked.
   */
  vtkSetClampMacro(TimerDuration, int, 1, 100000);
  vtkGetMacro(TimerDuration, int);
  ///@}

  /**
   * The method for activating and deactivating this widget. This method
   * must be overridden because it performs special timer-related operations.
   */
  void SetEnabled(int) override;

  /**
   * A default representation, of which there is none, is created. Note
   * that the superclasses vtkAbstractWidget::GetRepresentation()
   * method returns nullptr.
   */
  void CreateDefaultRepresentation() override { this->WidgetRep = nullptr; }

protected:
  vtkHoverWidget();
  ~vtkHoverWidget() override;

  // The state of the widget

  enum
  {
    Start = 0,
    Timing,
    TimedOut
  };

  int WidgetState;

  // Callback interface to execute events
  static void MoveAction(vtkAbstractWidget*);
  static void HoverAction(vtkAbstractWidget*);
  static void SelectAction(vtkAbstractWidget*);

  // Subclasses of this class invoke these methods. If a non-zero
  // value is returned, a subclass is handling the event.
  virtual int SubclassHoverAction() { return 0; }
  virtual int SubclassEndHoverAction() { return 0; }
  virtual int SubclassSelectAction() { return 0; }

  ///@{
  /**
   * Helper methods for creating and destroying timers.
   */
  int TimerId;
  int TimerDuration;
  ///@}

private:
  vtkHoverWidget(const vtkHoverWidget&) = delete;
  void operator=(const vtkHoverWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
