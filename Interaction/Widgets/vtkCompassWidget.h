// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkCompassWidget
 * @brief   widget to set distance, tilt and heading
 *
 * The vtkCompassWidget is used to adjust distance, tilt and heading parameters in an
 * application. It uses vtkCompassRepresentation as its representation.
 *
 * To customize the widget override the CreateDefaultRepresentation method and set the
 * representation to your own subclass of vtkCompassRepresentation. Ranges for distance and tilt can
 * be set in vtkCompassRepresentation.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 * If the slider bead is selected:
 *   LeftButtonPressEvent - select slider
 *   LeftButtonReleaseEvent - release slider
 *   MouseMoveEvent - move slider
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkCompassWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select -- some part of the widget has been selected
 *   vtkWidgetEvent::EndSelect -- the selection process has completed
 *   vtkWidgetEvent::Move -- a request for slider motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the vtkCompassWidget
 * invokes the following VTK events on itself (which observers can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
 *   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
 *   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
 *   vtkCommand::WidgetValueChangedEvent (when widget values have changed)
 * </pre>
 *
 */

#ifndef vtkCompassWidget_h
#define vtkCompassWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCompassRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkCompassWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate the class.
   */
  static vtkCompassWidget* New();

  ///@{
  /**
   * Standard macros.
   */
  vtkTypeMacro(vtkCompassWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkCompassRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  ///@{
  /**
   * Get/set the value for this widget.
   */
  double GetHeading();
  void SetHeading(double v);
  double GetTilt();
  void SetTilt(double tilt);
  double GetDistance();
  void SetDistance(double distance);
  ///@}

  ///@{
  /**
   * Get/set the timer interval in milliseconds. The timer interval determines the update frequency
   * for slider mouse interactions. Default is 50 ms.
   */
  vtkGetMacro(TimerDuration, int);
  vtkSetMacro(TimerDuration, int);
  ///@}

  ///@{
  /**
   * Get/set the tilt speed in degrees per second. This is the speed with which the tilt
   * changes when the top/bottom tilt slider button is clicked. Default is 30.0 degrees/s.
   */
  vtkGetMacro(TiltSpeed, double);
  vtkSetMacro(TiltSpeed, double);
  ///@}

  ///@{
  /**
   * Get/set the distance speed in distance per second. This is the speed with which the distance
   * changes when the top/bottom distance slider button is clicked. Default is 1.0/s.
   */
  vtkGetMacro(DistanceSpeed, double);
  vtkSetMacro(DistanceSpeed, double);
  ///@}

protected:
  vtkCompassWidget();
  ~vtkCompassWidget() override = default;

  // These are the events that are handled
  static void SelectAction(vtkAbstractWidget* widget);
  static void EndSelectAction(vtkAbstractWidget* widget);
  static void MoveAction(vtkAbstractWidget* widget);
  static void TimerAction(vtkAbstractWidget* widget);

  int WidgetState;
  enum WidgetStateType
  {
    Start = 0,
    Highlighting,
    Adjusting,
    TiltAdjusting,
    DistanceAdjusting,
    TiltTimerAdjustingDown,
    TiltTimerAdjustingUp,
    DistanceTimerAdjustingIn,
    DistanceTimerAdjustingOut
  };

  int TimerId = -1;
  int TimerDuration = 50;
  double StartTime;

  double TiltSpeed = 30.0;
  double DistanceSpeed = 1.0;

private:
  vtkCompassWidget(const vtkCompassWidget&) = delete;
  void operator=(const vtkCompassWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
