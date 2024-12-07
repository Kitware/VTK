// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkImplicitAnnulusWidget
 * @brief 3D widget for manipulating an infinite annulus
 *
 * This 3D widget defines an infinite annulus that can be
 * interactively placed in a scene. The widget is assumed to consist
 * of four parts: 1) an annulus contained in a 2) bounding box, with an
 * 3) annulus axis, which is rooted at an 4) origin point in the bounding
 * box. (The representation paired with this widget determines the
 * actual geometry of the widget.)
 *
 * To use this widget, you generally pair it with a vtkImplicitAnnulusRepresentation
 * (or a subclass). Various options are available for controlling how the
 * representation appears, and how the widget functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 *
 * If the annulus axis is selected:
 *  - LeftButtonPressEvent - select axis
 *  - LeftButtonReleaseEvent - release (end select) axis
 *  - MouseMoveEvent - orient the axis vector
 * If the center point handle is selected:
 *  - LeftButtonPressEvent - select handle (if on slider)
 *  - LeftButtonReleaseEvent - release handle (if selected)
 *  - MouseMoveEvent - move the center point (constrained to plane or on the
 *                    axis if CTRL key is pressed)
 * If the outline is selected:
 *  - LeftButtonPressEvent - select outline
 *  - LeftButtonReleaseEvent - release outline
 *  - MouseMoveEvent - move the outline
 * If the keypress characters are used
 *  - 'Down/Left' Move annulus away from viewer
 *  - 'Up/Right' Move annulus towards viewer
 * In all the cases, independent of what is picked, the widget responds to the
 * following VTK events:
 *  - MiddleButtonPressEvent - move the annulus
 *  - MiddleButtonReleaseEvent - release the annulus
 *  - RightButtonPressEvent - scale the widget's representation
 *  - RightButtonReleaseEvent - stop scaling the widget
 *  - MouseMoveEvent - scale (if right button) or move (if middle button) the widget
 *
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkImplicitAnnulusWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select -- some part of the widget has been selected
 *   vtkWidgetEvent::EndSelect -- the selection process has completed
 *   vtkWidgetEvent::Move -- a request for widget motion has been invoked
 *   vtkWidgetEvent::Up and vtkWidgetEvent::Down -- MoveAnnulusAction
 * </pre>
 *
 * In turn, when these widget events are processed, the vtkImplicitAnnulusWidget
 * invokes the following VTK events on itself (which observers can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
 *   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
 *   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
 * </pre>
 *
 *
 * @sa
 * vtk3DWidget vtkAnnulusRepresentation vtkAnnulus
 */

#ifndef vtkImplicitAnnulusWidget_h
#define vtkImplicitAnnulusWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkImplicitAnnulusRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkImplicitAnnulusWidget
  : public vtkAbstractWidget
{
public:
  static vtkImplicitAnnulusWidget* New();
  vtkTypeMacro(vtkImplicitAnnulusWidget, vtkAbstractWidget);

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkImplicitAnnulusRepresentation* rep);

  /**
   * Return the representation as a vtkImplicitAnnulusRepresentation.
   */
  vtkImplicitAnnulusRepresentation* GetAnnulusRepresentation()
  {
    return reinterpret_cast<vtkImplicitAnnulusRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

private:
  enum WidgetStateType
  {
    Idle = 0,
    Active
  };

  vtkImplicitAnnulusWidget();
  ~vtkImplicitAnnulusWidget() override = default;

  vtkImplicitAnnulusWidget(const vtkImplicitAnnulusWidget&) = delete;
  void operator=(const vtkImplicitAnnulusWidget&) = delete;

  ///@{
  /**
   * Callbacks for widget events
   */
  static void SelectAction(vtkAbstractWidget* widget);
  static void TranslateAction(vtkAbstractWidget* widget);
  static void ScaleAction(vtkAbstractWidget* widget);
  static void EndSelectAction(vtkAbstractWidget* widget);
  static void MoveAction(vtkAbstractWidget* widget);
  static void MoveAnnulusAction(vtkAbstractWidget* widget);
  static void TranslationAxisLock(vtkAbstractWidget* widget);
  static void TranslationAxisUnLock(vtkAbstractWidget* widget);
  /// @}

  /**
   * Update the cursor shape based on the interaction state. Returns true
   * if the cursor shape requested is different from the existing one.
   */
  bool UpdateCursorShape(int interactionState);

  // Manage the state of the widget
  WidgetStateType WidgetState = vtkImplicitAnnulusWidget::Idle;
};

VTK_ABI_NAMESPACE_END
#endif
