// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLightWidget
 * @brief   3D widget for showing a LightRepresentation
 *
 * To use this widget, one generally pairs it with a
 * vtkLightRepresentation. Various options are available in the representation
 * for controlling how the widget appears, and how it functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 * Select and move the sphere to change the light position.
 * Select and move the cone or the line to change the focal point.
 * Right-Click and scale on the cone to change the cone angle.
 * </pre>
 *
 * @warning
 * Note that the widget can be picked even when it is "behind"
 * other actors.  This is an intended feature and not a bug.
 *
 * @warning
 * This class, and vtkLightRepresentation, are second generation VTK widgets.
 *
 * @sa
 * vtkLightRepresentation vtkSphereWidget
 */

#ifndef vtkLightWidget_h
#define vtkLightWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkHandleWidget;
class vtkLightRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkLightWidget : public vtkAbstractWidget
{
public:
  static vtkLightWidget* New();
  vtkTypeMacro(vtkLightWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkLightRepresentation* r);

  /**
   * Return the representation as a vtkLightRepresentation.
   */
  vtkLightRepresentation* GetLightRepresentation();

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkLightWidget();
  ~vtkLightWidget() override = default;

  bool WidgetActive = false;

  // These methods handle events
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);

private:
  vtkLightWidget(const vtkLightWidget&) = delete;
  void operator=(const vtkLightWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
