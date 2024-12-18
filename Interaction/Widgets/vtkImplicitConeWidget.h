// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkImplicitConeWidget
 * @brief   3D widget for manipulating an infinite cone
 *
 * This 3D widget defines an infinite cone that can be
 * interactively placed in a scene. The widget is assumed to consist
 * of four parts: 1) a cone contained in a 2) bounding box, with a
 * 3) cone axis, which is rooted at an 4) origin point in the bounding
 * box. (The representation paired with this widget determines the
 * actual geometry of the widget.)
 *
 * To use this widget, you generally pair it with a vtkImplicitConeRepresentation
 * (or a subclass). Various options are available for controlling how the
 * representation appears, and how the widget functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * - If the cone axis is selected:
 *   - LeftButtonPressEvent - select normal
 *   - LeftButtonReleaseEvent - release (end select) normal
 *   - MouseMoveEvent - orient the normal vector
 * - If the origin point (handle) is selected:
 *   - LeftButtonPressEvent - select handle (if on slider)
 *   - LeftButtonReleaseEvent - release handle (if selected)
 *   - MouseMoveEvent - move the origin point (constrained to plane or on the
 *                    axis if CTRL key is pressed)
 * - If the cone is selected:
 *   - LeftButtonPressEvent - select cone
 *   - LeftButtonReleaseEvent - release cone
 *   - MouseMoveEvent - increase/decrease cone angle
 * - If the outline is selected:
 *   - LeftButtonPressEvent - select outline
 *   - LeftButtonReleaseEvent - release outline
 *   - MouseMoveEvent - move the outline
 * - If the keypress characters are used
 *   - 'Down/Left' Move cone away from viewer
 *   - 'Up/Right' Move cone towards viewer
 * - In all the cases, independent of what is picked, the widget responds to the
 * following VTK events:
 *   - MiddleButtonPressEvent - move the cone
 *   - MiddleButtonReleaseEvent - release the cone
 *   - RightButtonPressEvent - scale the widget's representation
 *   - RightButtonReleaseEvent - stop scaling the widget
 *   - MouseMoveEvent - scale (if right button) or move (if middle button) the widget
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkImplicitConeWidget's widget events:
 *   - vtkWidgetEvent::Select -- some part of the widget has been selected
 *   - vtkWidgetEvent::EndSelect -- the selection process has completed
 *   - vtkWidgetEvent::Move -- a request for widget motion has been invoked
 *   - vtkWidgetEvent::Up and vtkWidgetEvent::Down -- MoveConeAction
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the vtkImplicitConeWidget
 * invokes the following VTK events on itself (which observers can listen for):
 *   - vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
 *   - vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
 *   - vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
 *
 *
 * @sa
 * vtk3DWidget vtkImplicitPlaneWidget
 */

#ifndef vtkImplicitConeWidget_h
#define vtkImplicitConeWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkImplicitConeRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkImplicitConeWidget : public vtkAbstractWidget
{
public:
  static vtkImplicitConeWidget* New();
  vtkTypeMacro(vtkImplicitConeWidget, vtkAbstractWidget);

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkImplicitConeRepresentation* rep);

  /**
   * Return the representation as a vtkImplicitConeRepresentation.
   */
  vtkImplicitConeRepresentation* GetConeRepresentation()
  {
    return reinterpret_cast<vtkImplicitConeRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

private:
  vtkImplicitConeWidget();
  ~vtkImplicitConeWidget() override = default;

  enum WidgetStateType
  {
    Idle = 0,
    Active
  };

  // Manage the state of the widget
  WidgetStateType WidgetState = vtkImplicitConeWidget::Idle;

  // These methods handle events
  static void SelectAction(vtkAbstractWidget* widget);
  static void TranslateAction(vtkAbstractWidget* widget);
  static void ScaleAction(vtkAbstractWidget* widget);
  static void EndSelectAction(vtkAbstractWidget* widget);
  static void MoveAction(vtkAbstractWidget* widget);
  static void MoveConeAction(vtkAbstractWidget* widget);
  static void TranslationAxisLock(vtkAbstractWidget* widget);
  static void TranslationAxisUnLock(vtkAbstractWidget* widget);

  /**
   * Update the cursor shape based on the interaction state. Returns true
   * if the cursor shape requested is different from the existing one.
   */
  bool UpdateCursorShape(int interactionState);

  vtkImplicitConeWidget(const vtkImplicitConeWidget&) = delete;
  void operator=(const vtkImplicitConeWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
