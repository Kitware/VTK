// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSplineWidget2
 * @brief   widget for vtkSplineRepresentation.
 *
 * vtkSplineWidget2 is the vtkAbstractWidget subclass for
 * vtkSplineRepresentation which manages the interactions with
 * vtkSplineRepresentation. This is based on vtkSplineWidget.
 * @sa
 * vtkSplineRepresentation, vtkSplineWidget2
 */

#ifndef vtkSplineWidget2_h
#define vtkSplineWidget2_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkSplineRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkSplineWidget2 : public vtkAbstractWidget
{
public:
  static vtkSplineWidget2* New();
  vtkTypeMacro(vtkSplineWidget2, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of
   * vtkProp so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkSplineRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Override superclasses' SetEnabled() method because the line
   * widget must enable its internal handle widgets.
   */
  void SetEnabled(int enabling) override;

  /**
   * Create the default widget representation if one is not set. By default,
   * this is an instance of the vtkSplineRepresentation class.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkSplineWidget2();
  ~vtkSplineWidget2() override;

  int WidgetState;
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

  vtkCallbackCommand* KeyEventCallbackCommand;
  static void ProcessKeyEvents(vtkObject*, unsigned long, void*, void*);

private:
  vtkSplineWidget2(const vtkSplineWidget2&) = delete;
  void operator=(const vtkSplineWidget2&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
