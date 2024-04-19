// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCenteredSliderWidget
 * @brief   set a value by manipulating a slider
 *
 * The vtkCenteredSliderWidget is used to adjust a scalar value in an application.
 * This class measures deviations form the center point on the slider.
 * Moving the slider
 * modifies the value of the widget, which can be used to set parameters on
 * other objects. Note that the actual appearance of the widget depends on
 * the specific representation for the widget.
 *
 * To use this widget, set the widget representation. The representation is
 * assumed to consist of a tube, two end caps, and a slider (the details may
 * vary depending on the particulars of the representation). Then in the
 * representation you will typically set minimum and maximum value, as well
 * as the current value. The position of the slider must also be set, as well
 * as various properties.
 *
 * Note that the value should be obtain from the widget, not from the
 * representation. Also note that Minimum and Maximum values are in terms of
 * value per second. The value you get from this widget's GetValue method is
 * multiplied by time.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 * If the slider bead is selected:
 *   LeftButtonPressEvent - select slider (if on slider)
 *   LeftButtonReleaseEvent - release slider (if selected)
 *   MouseMoveEvent - move slider
 * If the end caps or slider tube are selected:
 *   LeftButtonPressEvent - move (or animate) to cap or point on tube;
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkCenteredSliderWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select -- some part of the widget has been selected
 *   vtkWidgetEvent::EndSelect -- the selection process has completed
 *   vtkWidgetEvent::Move -- a request for slider motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the vtkCenteredSliderWidget
 * invokes the following VTK events on itself (which observers can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
 *   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
 *   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
 * </pre>
 *
 */

#ifndef vtkCenteredSliderWidget_h
#define vtkCenteredSliderWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkSliderRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkCenteredSliderWidget
  : public vtkAbstractWidget
{
public:
  /**
   * Instantiate the class.
   */
  static vtkCenteredSliderWidget* New();

  ///@{
  /**
   * Standard macros.
   */
  vtkTypeMacro(vtkCenteredSliderWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkSliderRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a vtkSliderRepresentation.
   */
  vtkSliderRepresentation* GetSliderRepresentation()
  {
    return reinterpret_cast<vtkSliderRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Get the value fo this widget.
   */
  double GetValue() { return this->Value; }

protected:
  vtkCenteredSliderWidget();
  ~vtkCenteredSliderWidget() override = default;

  // These are the events that are handled
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void TimerAction(vtkAbstractWidget*);

  // Manage the state of the widget
  int WidgetState;
  enum WidgetStateType
  {
    Start = 0,
    Sliding
  };

  int TimerId;
  int TimerDuration;
  double StartTime;
  double Value;

private:
  vtkCenteredSliderWidget(const vtkCenteredSliderWidget&) = delete;
  void operator=(const vtkCenteredSliderWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
