// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSplineRepresentation
 * @brief   representation for a spline.
 *
 * vtkSplineRepresentation is a vtkWidgetRepresentation for a spline.
 * This 3D widget defines a spline that can be interactively placed in a
 * scene. The spline has handles, the number of which can be changed, plus it
 * can be picked on the spline itself to translate or rotate it in the scene.
 * This is based on vtkSplineWidget.
 * @sa
 * vtkSplineWidget, vtkSplineWidget2
 */

#ifndef vtkSplineRepresentation_h
#define vtkSplineRepresentation_h

#include "vtkAbstractSplineRepresentation.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkPointHandleSource;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkSplineRepresentation
  : public vtkAbstractSplineRepresentation
{
public:
  static vtkSplineRepresentation* New();
  vtkTypeMacro(vtkSplineRepresentation, vtkAbstractSplineRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the number of handles for this widget,
   *  while keeping a similar spline.
   */
  void SetNumberOfHandles(int npts) override;

  /**
   * Set the parametric spline object.
   */
  void SetParametricSpline(vtkParametricSpline* spline) override;

  /**
   * Convenience method to allocate and set the handles from a vtkPoints
   * instance.  If the first and last points are the same, the spline sets
   * Closed to the on InteractionState and disregards the last point, otherwise Closed
   * remains unchanged.
   */
  void InitializeHandles(vtkPoints* points) override;

  /**
   * Method that satisfy vtkWidgetRepresentation API.
   * Updates the spline in relation with the handles positions
   * and updates vtkWidgetRepresentation::InitialLength
   * (useful for the sizing methods).
   */
  void BuildRepresentation() override;

protected:
  vtkSplineRepresentation();
  ~vtkSplineRepresentation() override;

  /**
   * Specialized method to insert a handle on the spline.
   */
  int InsertHandleOnLine(double* pos) override;

  /**
   * Delete all the handles.
   */
  void ClearHandles();

  /**
   * Allocate/Reallocate the handles according
   * to npts.
   */
  void AllocateHandles(int npts);

  /**
   * Create npts default handles.
   */
  void CreateDefaultHandles(int npts);

  /**
   * Recreate the handles according to a
   * number of points equal to npts.
   * It uses the current spline to recompute
   * the positions of the new handles.
   */
  void ReconfigureHandles(int npts);

  // Specialized methods to access handles
  vtkActor* GetHandleActor(int index) override;
  vtkHandleSource* GetHandleSource(int index) override;
  int GetHandleIndex(vtkProp* prop) override;

private:
  vtkSplineRepresentation(const vtkSplineRepresentation&) = delete;
  void operator=(const vtkSplineRepresentation&) = delete;

  void RebuildRepresentation();

  // Glyphs representing hot spots (e.g., handles)
  std::vector<vtkSmartPointer<vtkPointHandleSource>> PointHandles;
  std::vector<vtkSmartPointer<vtkActor>> HandleActors;
};

VTK_ABI_NAMESPACE_END
#endif
