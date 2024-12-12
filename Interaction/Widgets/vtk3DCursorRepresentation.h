// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtk3DCursorRepresentation
 * @brief Representation of the vtk3DCursorWidget
 *
 * Internally, the class uses a vtkHardwarePicker to pick the position of the cursor
 * in the scene given a display position (in pixels). If the mouse hovers an actor,
 * the cursor is placed on its surface. If not, it's placed on the focal plane of the camera.
 * Because of the current state of pickers in VTK, this cursor do not support volumetric data.
 *
 * The cursor itself can be considered as a self-employed widget handle. For resizing the cursor,
 * use the SetHandleSize method of this widget.
 *
 * Current limitations :
 * - Do not work with volumes (for now no pickers handles them properly)
 * - Unsteady placement on other widgets (manipulation and cursor actualization remain fine)
 * - When zooming the cursor do not follows the mouse until moving it again
 *
 * @sa vtk3DCursorWidget
 */

#ifndef vtk3DCursorRepresentation_h
#define vtk3DCursorRepresentation_h

#include "vtkActor.h"                    // For vtkActor
#include "vtkHardwarePicker.h"           // For vtkHardwarePicker
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"                      // For vtkNew
#include "vtkSmartPointer.h"             // For vtkSmartPointer
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <memory> // for unique_ptr

VTK_ABI_NAMESPACE_BEGIN

class vtkViewport;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtk3DCursorRepresentation
  : public vtkWidgetRepresentation
{
public:
  static vtk3DCursorRepresentation* New();
  vtkTypeMacro(vtk3DCursorRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Update the cursor size (in world coordinates) to always keep the same
   * size in display coordinates.
   */
  void BuildRepresentation() override;

  /**
   * Position the cursor in the scene using the vtkHardwarePicker
   */
  void WidgetInteraction(double newEventPos[2]) override;

  ///@{
  /**
   * These methods are reimplemented to make this class behave as a vtkProp.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOpaqueGeometry(vtkViewport* viewport) override;
  ///@}

  enum CursorShape
  {
    CROSS_SHAPE = 0,
    SPHERE_SHAPE = 1,
    CUSTOM_SHAPE = 2
  };

  ///@{
  /**
   * Set / Get the shape of the cursor.
   * You can choose between CROSS, SPHERE and CUSTOM.
   * Choose CUSTOM if you want to use the actor you pass
   * with SetCustomCursor.
   */
  void SetCursorShape(int shape);
  vtkGetMacro(Shape, int);
  ///@}

  ///@{
  /**
   * Set / Get an actor to use as custom cursor.
   * You must set the cursor shape to CUSTOM enable it.
   */
  void SetCustomCursor(vtkActor* customCursor);
  vtkGetSmartPointerMacro(CustomCursor, vtkActor);
  ///@}

protected:
  vtk3DCursorRepresentation();
  ~vtk3DCursorRepresentation() override;

private:
  vtk3DCursorRepresentation(const vtk3DCursorRepresentation&) = delete;
  void operator=(const vtk3DCursorRepresentation&) = delete;

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  vtkSmartPointer<vtkActor> CustomCursor;
  int Shape = CROSS_SHAPE;
};

VTK_ABI_NAMESPACE_END
#endif
