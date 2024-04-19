// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVRPanelWidget
 * @brief   3D widget to display a panel/billboard
 *
 * Handles events for a PanelRepresentation.
 *
 * @sa
 * vtkVRPanelRepresentation
 */

#ifndef vtkVRPanelWidget_h
#define vtkVRPanelWidget_h

#include "vtkAbstractWidget.h"
#include "vtkRenderingVRModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkVRPanelRepresentation;

class VTKRENDERINGVR_EXPORT vtkVRPanelWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkVRPanelWidget* New();

  ///@{
  /**
   * Standard vtkObject methods
   */
  vtkTypeMacro(vtkVRPanelWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkVRPanelRepresentation* rep);

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkVRPanelWidget();
  ~vtkVRPanelWidget() override = default;

  // Manage the state of the widget
  int WidgetState;
  enum WidgetStateType
  {
    Start = 0,
    Active
  };

  /**
   * callback
   */
  static void SelectAction3D(vtkAbstractWidget*);
  static void EndSelectAction3D(vtkAbstractWidget*);
  static void MoveAction3D(vtkAbstractWidget*);

private:
  vtkVRPanelWidget(const vtkVRPanelWidget&) = delete;
  void operator=(const vtkVRPanelWidget&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
