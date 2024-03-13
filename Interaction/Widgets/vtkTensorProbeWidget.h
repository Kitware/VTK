// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTensorProbeWidget
 * @brief   a widget to probe tensors on a polyline
 *
 * The class is used to probe tensors on a trajectory. The representation
 * (vtkTensorProbeRepresentation) is free to choose its own method of
 * rendering the tensors. For instance vtkEllipsoidTensorProbeRepresentation
 * renders the tensors as ellipsoids. The interactions of the widget are
 * controlled by the left mouse button. A left click on the tensor selects
 * it. It can dragged around the trajectory to probe the tensors on it.
 *
 * For instance dragging the ellipsoid around with
 * vtkEllipsoidTensorProbeRepresentation will manifest itself with the
 * ellipsoid shape changing as needed along the trajectory.
 */

#ifndef vtkTensorProbeWidget_h
#define vtkTensorProbeWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkTensorProbeRepresentation;
class vtkPolyData;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkTensorProbeWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkTensorProbeWidget* New();

  ///@{
  /**
   * Standard VTK class macros.
   */
  vtkTypeMacro(vtkTensorProbeWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkTensorProbeRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a vtkTensorProbeRepresentation.
   */
  vtkTensorProbeRepresentation* GetTensorProbeRepresentation()
  {
    return reinterpret_cast<vtkTensorProbeRepresentation*>(this->WidgetRep);
  }

  /**
   * See vtkWidgetRepresentation for details.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkTensorProbeWidget();
  ~vtkTensorProbeWidget() override;

  // 1 when the probe has been selected, for instance when dragging it around
  int Selected;

  int LastEventPosition[2];

  // Callback interface to capture events and respond
  static void SelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);

private:
  vtkTensorProbeWidget(const vtkTensorProbeWidget&) = delete;
  void operator=(const vtkTensorProbeWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
