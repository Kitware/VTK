/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraPathWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkDeprecation.h"              // For VTK_DEPRECATED_IN_9_2_0
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"                      // for vtkNew

class vtkCameraPathRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT vtkCameraPathWidget : public vtkAbstractWidget
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
  ~vtkCameraPathWidget() override = default;

  int WidgetState = vtkCameraPathWidget::Start;
  enum WidgetStateType
  {
    Start = 0,
    Active
  };
#if !defined(VTK_LEGACY_REMOVE)
  VTK_DEPRECATED_IN_9_2_0("because leading underscore is reserved")
  typedef WidgetStateType _WidgetState;
#endif

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

#endif
