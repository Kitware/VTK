// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCameraPathWidget
 * @brief   widget for vtkCameraPathRepresentation.
 *
 * vtkCameraPathWidget is the vtkAbstractWidget subclass for
 * vtkCameraPathRepresentation which manages the interactions with
 * vtkCameraPathRepresentation. This is based on vtkSplineWidget2.
 * @sa
 * vtkCameraPathRepresentation
 */

#ifndef vtkCameraPathWidget_h
#define vtkCameraPathWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"                      // for vtkNew
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCameraPathRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkCameraPathWidget : public vtkAbstractWidget
{
public:
  static vtkCameraPathWidget* New();
  vtkTypeMacro(vtkCameraPathWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of
   * vtkProp so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkCameraPathRepresentation* r);

  /**
   * Override superclasses SetEnabled() method because the line
   * widget must enable its internal handle widgets.
   */
  void SetEnabled(int enabling) override;

  /**
   * Create the default widget representation if one is not set. By default,
   * this is an instance of the vtkCameraPathRepresentation class.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkCameraPathWidget();
  ~vtkCameraPathWidget() override;

  int WidgetState = vtkCameraPathWidget::Start;
  enum WidgetStateType
  {
    Start = 0,
    Active
  };

  // These methods handle events
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void TranslateAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

  vtkNew<vtkCallbackCommand> KeyEventCallbackCommand;
  static void ProcessKeyEvents(vtkObject*, unsigned long, void*, void*);

private:
  vtkCameraPathWidget(const vtkCameraPathWidget&) = delete;
  void operator=(const vtkCameraPathWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
