// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyLineRepresentation
 * @brief   vtkWidgetRepresentation for a poly line.
 *
 * vtkPolyLineRepresentation is a vtkCurveRepresentation for a poly
 * line. This 3D widget defines a poly line that can be interactively
 * placed in a scene. The poly line has handles, the number of which
 * can be changed, plus the widget can be picked on the poly line
 * itself to translate or rotate it in the scene.
 * Based on vtkCurveRepresentation
 * @sa
 * vtkSplineRepresentation
 */

#ifndef vtkPolyLineRepresentation_h
#define vtkPolyLineRepresentation_h

#include "vtkCurveRepresentation.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyLineSource;
class vtkPointHandleSource;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkPolyLineRepresentation
  : public vtkCurveRepresentation
{
public:
  static vtkPolyLineRepresentation* New();
  vtkTypeMacro(vtkPolyLineRepresentation, vtkCurveRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Grab the polydata (including points) that defines the poly line.
   * polydata consists of points and line segments between consecutive
   * points. Points are guaranteed to be up-to-date when either the
   * InteractionEvent or EndInteraction events are invoked. The user
   * provides the vtkPolyData and the points and polyline are added to
   * it.
   */
  void GetPolyData(vtkPolyData* pd) override;

  /**
   * Set the number of handles for this widget.
   */
  void SetNumberOfHandles(int npts) override;

  /**
   * Get the positions of the handles.
   */
  vtkDoubleArray* GetHandlePositions() override;

  /**
   * Get the true length of the poly line. Calculated as the summed
   * lengths of the individual straight line segments.
   */
  double GetSummedLength() override;

  /**
   * Convenience method to allocate and set the handles from a
   * vtkPoints instance.  If the first and last points are the same,
   * the poly line sets Closed to on and disregards the last point,
   * otherwise Closed remains unchanged.
   */
  void InitializeHandles(vtkPoints* points) override;

  /**
   * Build the representation for the poly line.
   */
  void BuildRepresentation() override;

protected:
  vtkPolyLineRepresentation();
  ~vtkPolyLineRepresentation() override;

  // The poly line source
  vtkNew<vtkPolyLineSource> PolyLineSource;

  /**
   * Specialized method to insert a handle on the poly line.
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
  vtkPolyLineRepresentation(const vtkPolyLineRepresentation&) = delete;
  void operator=(const vtkPolyLineRepresentation&) = delete;

  // Glyphs representing hot spots (e.g., handles)
  std::vector<vtkSmartPointer<vtkPointHandleSource>> PointHandles;
  std::vector<vtkSmartPointer<vtkActor>> HandleActors;
};

VTK_ABI_NAMESPACE_END
#endif
