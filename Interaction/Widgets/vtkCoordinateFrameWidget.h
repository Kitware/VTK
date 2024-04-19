// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCoordinateFrameWidget
 * @brief   3D widget for manipulating a display sized coordinate frame widget
 *
 * This 3D widget defines a display sized coordinate frame
 * that can be interactively placed in a scene. The widget is assumed
 * to consist of three parts: 1) an origin, 2) 3 axis normals which are rooted at the origin
 * and 3 axis lockers. (The representation paired with this widget determines the actual
 * geometry of the widget.)
 *
 * To use this widget, you generally pair it with a vtkCoordinateFrameRepresentation
 * (or a subclass). Various options are available for controlling how the
 * representation appears, and how the widget functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 * If the mouse is over the one of the axis normals:
 *   LeftButtonPressEvent - select normal
 *   LeftButtonReleaseEvent - release normal
 *   MouseMoveEvent - orient the axis normal vectors (possibly constrained to
 *   one of the axis' planes)
 * If the mouse is over the origin point (handle):
 *   LeftButtonPressEvent - select handle
 *   LeftButtonReleaseEvent - release handle (if selected)
 *   MouseMoveEvent - move the origin point (possibly constrained to one of the axis' planes)
 * If the mouse is over one of the axis lockers:
 *   LeftButtonPressEvent - select axis locker
 *   LeftButtonReleaseEvent - unlock/lock and axis locker (and unlock all the other lockers)
 * If the keypress characters are used
 *   'Down/Left' Move plane down
 *   'Up/Right' Move plane up
 *   'P/p' Pick a new origin from the intersection with a mesh cell rendered by the renderer
 *   'Ctrl' + 'P/p' Snap to a new origin from the closest mesh point rendered by the renderer
 *   'N/n' Pick a new normal from the intersection with a mesh cell rendered by the renderer
 *   'Ctrl' + 'N/n' Snap to a new normal from the closest mesh point rendered by the renderer
 *   'D/d' Pick a new point to define the direction normal which will be the new normal
 *   'Ctrl' + 'D/d' Snap to a new point to define the direction normal which will be the new normal
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkCoordinateFrameWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select -- some part of the widget has been selected
 *   vtkWidgetEvent::EndSelect -- the selection process has completed
 *   vtkWidgetEvent::Move -- a request for widget motion has been invoked
 *   vtkWidgetEvent::PickPoint -- PickOriginAction
 *   vtkWidgetEvent::PickNormal -- PickNormalAction
 *   vtkWidgetEvent::PickDirectionPoint -- PickDirectionPointAction
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the vtkCoordinateFrameWidget
 * invokes the following VTK events on itself (which observers can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
 *   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
 *   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
 * </pre>
 *
 *
 * @par Event Bindings:
 * This class, and vtkCoordinateFrameRepresentation, are next generation VTK
 * widgets.
 *
 * @sa
 * vtk3DWidget vtkBoxWidget vtkPlaneWidget vtkLineWidget vtkPointWidget
 * vtkSphereWidget vtkImagePlaneWidget vtkImplicitCylinderWidget, vtkImplicitPlaneWidget2
 * vtkDisplaySizedImplicitPlaneWidget
 */

#ifndef vtkCoordinateFrameWidget_h
#define vtkCoordinateFrameWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCoordinateFrameRepresentation;
class vtkCoordinateFrameWidgetInteractionCallback;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkCoordinateFrameWidget
  : public vtkAbstractWidget
{
  friend class vtkCoordinateFrameWidgetInteractionCallback;

public:
  /**
   * Instantiate the object.
   */
  static vtkCoordinateFrameWidget* New();

  ///@{
  /**
   * Standard vtkObject methods
   */
  vtkTypeMacro(vtkCoordinateFrameWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkCoordinateFrameRepresentation* rep);

  // Description:
  // Disable/Enable the widget if needed.
  // Unobserved the camera if the widget is disabled.
  void SetEnabled(int enabling) override;

  /**
   * Return the representation as a vtkCoordinateFrameRepresentation.
   */
  vtkCoordinateFrameRepresentation* GetCoordinateFrameRepresentation()
  {
    return reinterpret_cast<vtkCoordinateFrameRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkCoordinateFrameWidget();
  ~vtkCoordinateFrameWidget() override;

  // Manage the state of the widget
  int WidgetState;
  enum WidgetStateType
  {
    Start = 0,
    Active
  };

  // These methods handle events
  static void SelectAction(vtkAbstractWidget*);
  static void PickOriginAction(vtkAbstractWidget*);
  static void PickNormalAction(vtkAbstractWidget*);
  static void PickDirectionPointAction(vtkAbstractWidget* w);
  static void TranslateAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void TranslationAxisLock(vtkAbstractWidget*);
  static void TranslationAxisUnLock(vtkAbstractWidget*);

  /**
   * Update the cursor shape based on the interaction state. Returns 1
   * if the cursor shape requested is different from the existing one.
   */
  int UpdateCursorShape(int interactionState);

  ///@{
  /**
   * Handle the interaction callback that may come from the representation.
   */
  vtkCoordinateFrameWidgetInteractionCallback* InteractionCallback;
  void InvokeInteractionCallback();
  ///@}
private:
  vtkCoordinateFrameWidget(const vtkCoordinateFrameWidget&) = delete;
  void operator=(const vtkCoordinateFrameWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
