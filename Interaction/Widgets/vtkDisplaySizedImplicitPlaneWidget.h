// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDisplaySizedImplicitPlaneWidget
 * @brief   3D widget for manipulating a display sized plane
 *
 * This 3D widget defines a display sized plane represented as a disk,
 * that can be interactively placed in a scene. The widget is assumed
 * to consist of three parts: 1) a disk plane with a 2) plane normal, which
 * is rooted at a 3) point on the plane. (The representation paired
 * with this widget determines the actual geometry of the widget.)
 *
 * To use this widget, you generally pair it with a vtkDisplaySizedImplicitPlaneRepresentation
 * (or a subclass). Various options are available for controlling how the
 * representation appears, and how the widget functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 * If the mouse is over the plane normal:
 *   LeftButtonPressEvent - select normal
 *   LeftButtonReleaseEvent - release normal
 *   MouseMoveEvent - orient the normal vector
 * If the mouse is over the origin point (handle):
 *   LeftButtonPressEvent - select handle
 *   LeftButtonReleaseEvent - release handle (if selected)
 *   MouseMoveEvent - move the origin point (constrained to the plane)
 * If the mouse is over the plane:
 *   LeftButtonPressEvent - select plane
 *   LeftButtonReleaseEvent - release plane (if selected)
 *   MouseMoveEvent - move the plane
 * If the mouse is over the perimeter of the disk plane:
 *   LeftButtonPressEvent - select perimeter
 *   LeftButtonReleaseEvent - release perimeter (if selected)
 *   MouseMoveEvent - resize the perimeter -> radius of the disk plane
 * If the keypress characters are used
 *   'Down/Left' Move plane down
 *   'Up/Right' Move plane up
 *   'P/p' Pick a new origin from the intersection with a mesh cell rendered by the renderer
 *   'Ctrl' + 'P/p' Snap to a new origin from the closest mesh point rendered by the renderer
 *   'N/n' Pick a new normal from the intersection with a mesh cell rendered by the renderer
 *   'Ctrl' + 'N/n' Snap to a new normal from the closest mesh point rendered by the renderer
 * In all the cases, independent of what is picked, the widget responds to the following VTK events:
 *   MiddleButtonPressEvent - move the plane
 *   MiddleButtonReleaseEvent - release the plane
 *   RightButtonPressEvent - scale the widget's representation
 *   RightButtonReleaseEvent - stop scaling the widget
 *   MouseMoveEvent - scale (if right button) or move (if middle button) the widget
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkDisplaySizedImplicitPlaneWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select -- some part of the widget has been selected
 *   vtkWidgetEvent::EndSelect -- the selection process has completed
 *   vtkWidgetEvent::Move -- a request for widget motion has been invoked
 *   vtkWidgetEvent::Up and vtkWidgetEvent::Down -- MovePlaneAction
 *   vtkWidgetEvent::PickPoint -- PickOriginAction
 *   vtkWidgetEvent::PickNormal -- PickNormalAction
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the vtkDisplaySizedImplicitPlaneWidget
 * invokes the following VTK events on itself (which observers can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
 *   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
 *   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
 * </pre>
 *
 *
 * @par Event Bindings:
 * This class, and vtkDisplaySizedImplicitPlaneRepresentation, are next generation VTK
 * widgets.
 *
 * @sa
 * vtk3DWidget vtkBoxWidget vtkPlaneWidget vtkLineWidget vtkPointWidget
 * vtkSphereWidget vtkImagePlaneWidget vtkImplicitCylinderWidget, vtkImplicitPlaneWidget2
 */

#ifndef vtkDisplaySizedImplicitPlaneWidget_h
#define vtkDisplaySizedImplicitPlaneWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkDisplaySizedImplicitPlaneRepresentation;
class vtkDisplaySizedImplicitPlaneInteractionCallback;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkDisplaySizedImplicitPlaneWidget
  : public vtkAbstractWidget
{
  friend class vtkDisplaySizedImplicitPlaneInteractionCallback;

public:
  /**
   * Instantiate the object.
   */
  static vtkDisplaySizedImplicitPlaneWidget* New();

  ///@{
  /**
   * Standard vtkObject methods
   */
  vtkTypeMacro(vtkDisplaySizedImplicitPlaneWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkDisplaySizedImplicitPlaneRepresentation* rep);

  // Description:
  // Disable/Enable the widget if needed.
  // Unobserved the camera if the widget is disabled.
  void SetEnabled(int enabling) override;

  /**
   * Observe/Unobserve the camera if the widget is locked/unlocked to update the
   * vtkImplicitePlaneRepresentation's normal.
   */
  void SetLockNormalToCamera(int lock);

  /**
   * Return the representation as a vtkDisplaySizedImplicitPlaneRepresentation.
   */
  vtkDisplaySizedImplicitPlaneRepresentation* GetDisplaySizedImplicitPlaneRepresentation()
  {
    return reinterpret_cast<vtkDisplaySizedImplicitPlaneRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkDisplaySizedImplicitPlaneWidget();
  ~vtkDisplaySizedImplicitPlaneWidget() override;

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
  static void TranslateAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void MovePlaneAction(vtkAbstractWidget*);
  static void SelectAction3D(vtkAbstractWidget*);
  static void EndSelectAction3D(vtkAbstractWidget*);
  static void MoveAction3D(vtkAbstractWidget*);
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
  vtkDisplaySizedImplicitPlaneInteractionCallback* InteractionCallback;
  void InvokeInteractionCallback();
  ///@}

private:
  vtkDisplaySizedImplicitPlaneWidget(const vtkDisplaySizedImplicitPlaneWidget&) = delete;
  void operator=(const vtkDisplaySizedImplicitPlaneWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
