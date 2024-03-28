// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCamera3DWidget
 * @brief   3D Widget for manipulating a vtkCamera
 *
 * This 3D widget interacts with a vtkCamera3DRepresentation class (i.e., it
 * handles the events that drive its corresponding representation). A nice
 * feature of vtkCamera3DWidget, like any 3D widget, will work with the current
 * interactor style. That is, if vtkCamera3DWidget does not handle an event,
 * then all other registered observers (including the interactor style) have
 * an opportunity to process the event. Otherwise, the vtkCamera3DWidget will
 * terminate the processing of the event that it handles.
 *
 * To use this widget, you pair it with a vtkCamera3DRepresentation
 * (or a subclass). Various options are available in the representation for
 * controlling how the widget appears, and how the widget functions.
 *
 * @par Mouse Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 * Select and move the camera box to change the camera position.
 * Select and move the camera cone to change the camera view angle.
 * Select and move the sphere handles to change the target and view up.
 * </pre>
 *
 * @par Key Event Bindings:
 * By default, the widget responds to the following key pressed event:
 * <pre>
 * 'x' or 'X': set the translation constrained to X axis, or None if
 * already set to X.
 * 'y' or 'Y': set the translation constrained to Y axis, or None if
 * already set to Y.
 * 'z' or 'Z': set the translation constrained to Z axis, or None if
 * already set to Z.
 * 'o' or 'O': remove any translation constraint.
 * 'a' or 'A': toggle translation of both position and target, or only
 * one at a time.
 * 'c' or 'C': toggle frustum visibility.
 * </pre>
 *
 * @warning
 * This class, and vtkCamera3DRepresentation, are second generation VTK widgets.
 *
 * @sa
 * vtkCamera3DRepresentation
 */

#ifndef vtkCamera3DWidget_h
#define vtkCamera3DWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"                      // For vtkNew
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCamera;
class vtkCamera3DRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkCamera3DWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkCamera3DWidget* New();

  vtkTypeMacro(vtkCamera3DWidget, vtkAbstractWidget);

  /**
   * Create the default widget representation if one is not set. By default,
   * this is an instance of the vtkCamera3DRepresentation class.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independently of the widget.
   */
  void SetRepresentation(vtkCamera3DRepresentation* r);

  /**
   * Override superclasses' SetEnabled() method for
   * key event registration.
   */
  void SetEnabled(int enabling) override;

protected:
  vtkCamera3DWidget();
  ~vtkCamera3DWidget() override;

  bool Active = false;
  vtkNew<vtkCallbackCommand> KeyEventCallbackCommand;

  // These methods handle events
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

  static void ProcessKeyEvents(vtkObject*, unsigned long, void*, void*);

private:
  vtkCamera3DWidget(const vtkCamera3DWidget&) = delete;
  void operator=(const vtkCamera3DWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
