/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DCursorRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include <memory> // for unique_ptr

VTK_ABI_NAMESPACE_BEGIN

class vtkViewport;

class VTKINTERACTIONWIDGETS_EXPORT vtk3DCursorRepresentation : public vtkWidgetRepresentation
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

  ///@{
  /**
   * Set / Get the actor currently used as the 3D cursor.
   * By default, the cursor is a 3D cross (vtkCursor3D).
   */
  vtkSetSmartPointerMacro(Cursor, vtkActor);
  vtkGetSmartPointerMacro(Cursor, vtkActor);
  ///@}

protected:
  vtk3DCursorRepresentation();
  ~vtk3DCursorRepresentation() override;

  vtkSmartPointer<vtkActor> Cursor;

private:
  vtk3DCursorRepresentation(const vtk3DCursorRepresentation&) = delete;
  void operator=(const vtk3DCursorRepresentation&) = delete;

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
