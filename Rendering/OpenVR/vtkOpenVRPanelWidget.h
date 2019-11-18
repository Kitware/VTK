/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRPanelWidget.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRPanelWidget
 * @brief   3D widget to display a panel/billboard
 *
 * Handles events for a PanelRepresentation.
 *
 * @sa
 * vtkOpenVRPanelRepresentation
 */

#ifndef vtkOpenVRPanelWidget_h
#define vtkOpenVRPanelWidget_h

#include "vtkAbstractWidget.h"
#include "vtkRenderingOpenVRModule.h" // For export macro

class vtkOpenVRPanelRepresentation;
class vtkPropMap;
class vtkProp;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRPanelWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkOpenVRPanelWidget* New();

  //@{
  /**
   * Standard vtkObject methods
   */
  vtkTypeMacro(vtkOpenVRPanelWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkOpenVRPanelRepresentation* rep);

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkOpenVRPanelWidget();
  ~vtkOpenVRPanelWidget() override;

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
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
  vtkOpenVRPanelWidget(const vtkOpenVRPanelWidget&) = delete;
  void operator=(const vtkOpenVRPanelWidget&) = delete;
};
#endif
