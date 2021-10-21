/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointCloudWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointCloudWidget
 * @brief   select and manipulate a point from a point cloud
 *
 * This 3D widget allows the user to query and select one point from a large
 * collection of points (i.e., a point cloud). It is typically paired with a
 * representation class such as vtkPointCloudRepresentation.
 *
 * The widget emits several events during usage. When a point is successfully
 * picked (via mousing over), it emits the vtkCommand::PickEvent. When a
 * selection occurs, the vtkCommand::WidgetActivateEvent is invoked.
 *
 * In usage this widget is typically paired with an additional widget to
 * provide local point editing (e.g., move a point, modify associated
 * attribute values); or uses callbacks (via the Observer mechanism) to
 * perform additional operations on the selected point.
 *
 * Typically the behavior of the widget is as follows. As the user mouses
 * over the point cloud, picking continuously occurs. If a point is 1) picked
 * (i.e., the cursor is within a tolerance) and 2) a different point is
 * picked, then a widget pick event is invoked. The user, by observing the
 * event, can display further information such as text strings
 * etc. Additionally, the user can select the point (typically a left mouse
 * button click) which invokes a widget select event.
 *
 * @sa
 * vtkPointCloudRepresentation vtkPointWidget vtkTensorSizingWidget
 */

#ifndef vtkPointCloudWidget_h
#define vtkPointCloudWidget_h

#include "vtkAbstractWidget.h"
#include "vtkDeprecation.h"              // For VTK_DEPRECATED_IN_9_2_0
#include "vtkInteractionWidgetsModule.h" // For export macro

class vtkPointCloudRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT vtkPointCloudWidget : public vtkAbstractWidget
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkPointCloudWidget* New();
  vtkTypeMacro(vtkPointCloudWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkPointCloudRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Create the default widget representation if one is not set. By default,
   * this is an instance of the vtkPointCloudRepresentation class.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Override superclasses' SetEnabled() method because the line
   * widget must enable its internal handle widgets.
   */
  void SetEnabled(int enabling) override;

protected:
  vtkPointCloudWidget();
  ~vtkPointCloudWidget() override;

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

  // These methods handle mouse events
  static void MoveAction(vtkAbstractWidget*);
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);

private:
  vtkPointCloudWidget(const vtkPointCloudWidget&) = delete;
  void operator=(const vtkPointCloudWidget&) = delete;
};

#endif
