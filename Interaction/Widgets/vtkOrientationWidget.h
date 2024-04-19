// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOrientationWidget
 * @brief   3D Widget for manipulating a vtkCamera
 *
 * This 3D widget interacts with a vtkOrientationRepresentation class (i.e., it
 * handles the events that drive its corresponding representation). A nice
 * feature of vtkOrientationWidget, like any 3D widget, will work with the current
 * interactor style. That is, if vtkOrientationWidget does not handle an event,
 * then all other registered observers (including the interactor style) have
 * an opportunity to process the event. Otherwise, the vtkOrientationWidget will
 * terminate the processing of the event that it handles.
 *
 * To use this widget, you pair it with a vtkOrientationRepresentation
 * (or a subclass). Various options are available in the representation for
 * controlling how the widget appears, and how the widget functions.
 *
 * @par Mouse Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 * Select and move a torus to update the associated orientation.
 * Select and move an arrow to update the associated orientation.
 * </pre>
 *
 * @warning
 * This class, and vtkOrientationRepresentation, are second generation VTK widgets.
 *
 * @sa
 * vtkOrientationRepresentation
 */

#ifndef vtkOrientationWidget_h
#define vtkOrientationWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"                      // For vtkNew
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOrientationRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkOrientationWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkOrientationWidget* New();

  vtkTypeMacro(vtkOrientationWidget, vtkAbstractWidget);

  /**
   * Create the default widget representation if one is not set. By default,
   * this is an instance of the vtkOrientationRepresentation class.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independently of the widget.
   */
  void SetRepresentation(vtkOrientationRepresentation* r);

protected:
  vtkOrientationWidget();
  ~vtkOrientationWidget() override;

  // These methods handle events
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

private:
  bool Active = false;

  vtkOrientationWidget(const vtkOrientationWidget&) = delete;
  void operator=(const vtkOrientationWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
