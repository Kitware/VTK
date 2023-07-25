// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQWidgetWidget
 * @brief   3D VTK widget for a QWidget
 *
 * This 3D widget handles events between VTK and Qt for a QWidget placed
 * in a scene. It currently takes 6dof events as from VR controllers and
 * if they intersect the widget it converts them to Qt events and fires
 * them off.
 */

#ifndef vtkQWidgetWidget_h
#define vtkQWidgetWidget_h

#include "vtkAbstractWidget.h"
#include "vtkDeprecation.h"        // For VTK_DEPRECATED_IN_9_2_0
#include "vtkGUISupportQtModule.h" // For export macro
#include <QPointF>                 // for ivar

class QWidget;

VTK_ABI_NAMESPACE_BEGIN
class vtkQWidgetRepresentation;

class VTKGUISUPPORTQT_EXPORT vtkQWidgetWidget : public vtkAbstractWidget
{
  friend class vtkInteractionCallback;

public:
  /**
   * Instantiate the object.
   */
  static vtkQWidgetWidget* New();

  ///@{
  /**
   * Standard vtkObject methods
   */
  vtkTypeMacro(vtkQWidgetWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkQWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkQWidgetRepresentation* rep);

  // Description:
  // Disable/Enable the widget if needed.
  // Unobserved the camera if the widget is disabled.
  void SetEnabled(int enabling) override;

  /**
   * Return the representation as a vtkQWidgetRepresentation
   */
  vtkQWidgetRepresentation* GetQWidgetRepresentation();

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Set the QWidget that will receive the events.
   */
  void SetWidget(QWidget* w);
  QWidget* GetWidget() { return this->Widget; }

protected:
  vtkQWidgetWidget();
  ~vtkQWidgetWidget() override;

  // Manage the state of the widget
  int WidgetState;
  enum WidgetStateType
  {
    Start = 0,
    Active
  };
#if !defined(VTK_LEGACY_REMOVE)
  VTK_DEPRECATED_IN_9_2_0("because leading underscore is reserved")
  typedef WidgetStateType _WidgetState;
#endif

  QWidget* Widget;
  QPointF LastWidgetCoordinates;
  QPointF SteadyWidgetCoordinates;
  double SelectStartTime;

  // These methods handle events
  static void SelectAction3D(vtkAbstractWidget*);
  static void EndSelectAction3D(vtkAbstractWidget*);
  static void MoveAction3D(vtkAbstractWidget*);

private:
  vtkQWidgetWidget(const vtkQWidgetWidget&) = delete;
  void operator=(const vtkQWidgetWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
