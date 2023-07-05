// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCameraWidget
 * @brief   2D widget for saving a series of camera views
 *
 * This class provides support for interactively saving a series of camera
 * views into an interpolated path (using vtkCameraInterpolator). To use the
 * class start by specifying a camera to interpolate, and then simply start
 * recording by hitting the "record" button, manipulate the camera (by using
 * an interactor, direct scripting, or any other means), and then save the
 * camera view. Repeat this process to record a series of views.  The user
 * can then play back interpolated camera views using the
 * vtkCameraInterpolator.
 *
 * @sa
 * vtkBorderWidget vtkCameraInterpolator
 */

#ifndef vtkCameraWidget_h
#define vtkCameraWidget_h

#include "vtkBorderWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkCameraRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT vtkCameraWidget : public vtkBorderWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkCameraWidget* New();

  ///@{
  /**
   * Standard VTK class methods.
   */
  vtkTypeMacro(vtkCameraWidget, vtkBorderWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkCameraRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkCameraWidget();
  ~vtkCameraWidget() override;

  /**
   * When selecting the interior of this widget, special operations occur
   * (i.e., adding a camera view, deleting a path, animating a path). Thus
   * this methods overrides the superclasses' method.
   */
  void SelectRegion(double eventPos[2]) override;

private:
  vtkCameraWidget(const vtkCameraWidget&) = delete;
  void operator=(const vtkCameraWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
