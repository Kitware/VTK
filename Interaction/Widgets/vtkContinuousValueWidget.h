// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkContinuousValueWidget
 * @brief   set a value by manipulating something
 *
 * The vtkContinuousValueWidget is used to adjust a scalar value in an
 * application. Note that the actual appearance of the widget depends on
 * the specific representation for the widget.
 *
 * To use this widget, set the widget representation. (the details may
 * vary depending on the particulars of the representation).
 *
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
 * into the vtkContinuousValueWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select -- some part of the widget has been selected
 *   vtkWidgetEvent::EndSelect -- the selection process has completed
 *   vtkWidgetEvent::Move -- a request for slider motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the vtkContinuousValueWidget
 * invokes the following VTK events on itself (which observers can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
 *   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
 *   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
 * </pre>
 *
 */

#ifndef vtkContinuousValueWidget_h
#define vtkContinuousValueWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkContinuousValueWidgetRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkContinuousValueWidget
  : public vtkAbstractWidget
{
public:
  ///@{
  /**
   * Standard macros.
   */
  vtkTypeMacro(vtkContinuousValueWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkContinuousValueWidgetRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a vtkContinuousValueWidgetRepresentation.
   */
  vtkContinuousValueWidgetRepresentation* GetContinuousValueWidgetRepresentation()
  {
    return reinterpret_cast<vtkContinuousValueWidgetRepresentation*>(this->WidgetRep);
  }

  ///@{
  /**
   * Get the value for this widget.
   */
  double GetValue();
  void SetValue(double v);
  ///@}

protected:
  vtkContinuousValueWidget();
  ~vtkContinuousValueWidget() override = default;

  // These are the events that are handled
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

  // Manage the state of the widget
  int WidgetState;
  enum WidgetStateType
  {
    Start = 0,
    Highlighting,
    Adjusting
  };

  double Value;

private:
  vtkContinuousValueWidget(const vtkContinuousValueWidget&) = delete;
  void operator=(const vtkContinuousValueWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
