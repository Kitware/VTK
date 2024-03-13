// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtk3DCursorWidget
 * @brief   Widget reprensenting a 3D cursor.
 *
 * This cursor is primarily intended to be used when doing stereo rendering.
 * The cursor is represented by an actor that is added to the scene,
 * and hence can be rendered in stereo like other actors.
 * The vtk3DCursorRepresentation class handles the placement of the cursor in the scene,
 * given the display position of the mouse.
 *
 * @sa vtk3DCursorRepresentation
 */

#ifndef vtk3DCursorWidget_h
#define vtk3DCursorWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtk3DCursorRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtk3DCursorWidget : public vtkAbstractWidget
{
public:
  static vtk3DCursorWidget* New();
  vtkTypeMacro(vtk3DCursorWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that vtkWidgetRepresentation is a subclass
   * of vtkProp so it can be also added to the renderer directly.
   */
  void SetRepresentation(vtk3DCursorRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Return the widget representation as a vtk3DCursorRepresentation.
   */
  vtk3DCursorRepresentation* Get3DCursorRepresentation()
  {
    return reinterpret_cast<vtk3DCursorRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if no one is set. By default
   * an instance of vtk3DCursorRepresentation is created.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtk3DCursorWidget();
  ~vtk3DCursorWidget() override = default;

  /**
   * Callback function used to place the cursor in the scene using the picker.
   */
  static void MoveAction(vtkAbstractWidget*);

private:
  vtk3DCursorWidget(const vtk3DCursorWidget&) = delete;
  void operator=(const vtk3DCursorWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
