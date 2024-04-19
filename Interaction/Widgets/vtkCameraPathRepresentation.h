// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCameraPathRepresentation
 * @brief   representation for a camera path.
 *
 * vtkCameraPathRepresentation is a vtkWidgetRepresentation for a camera path.
 * This 3D widget defines a camera path that can be interactively manipulated in a
 * scene. The camera path has camera handles, that can be added and deleted, plus they
 * can be picked on the path itself to be translated in the scene.
 * This representation is used with vtkCameraPathWidget.
 * @sa
 * vtkCameraPathWidget
 */

#ifndef vtkCameraPathRepresentation_h
#define vtkCameraPathRepresentation_h

#include "vtkAbstractSplineRepresentation.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCameraHandleSource;
class vtkCamera;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkCameraPathRepresentation
  : public vtkAbstractSplineRepresentation
{
public:
  static vtkCameraPathRepresentation* New();
  vtkTypeMacro(vtkCameraPathRepresentation, vtkAbstractSplineRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Sets the representation to be a curve interpolating multiple cameras pointing to their focal
   * point. If set to false, the cameras are only represented as spheres.
   */
  void SetDirectional(bool val) override;

  /**
   * Adjust the number of camera handles while keeping
     the same path.
   */
  void SetNumberOfHandles(int npts) override;

  /**
   * Set the parametric spline object.
   */
  void SetParametricSpline(vtkParametricSpline* spline) override;

  /**
   * Convenience method to allocate and set the Camera handles from a vtkPoints
   * instance (corresponding to the camera positions).
   * If the first and last points are the same, the spline sets
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

  /**
   * Add a camera to the path.
   */
  void AddCameraAt(vtkCamera* camera, int index);

  /**
   * Remove a camera from the path.
   */
  void DeleteCameraAt(int index);

protected:
  vtkCameraPathRepresentation();
  ~vtkCameraPathRepresentation() override = default;

  /**
   * Resize the camera handles on demand.
   */
  void SizeHandles() override;

  /**
   * Delete all camera handles.
   */
  void ClearCameraHandles();

  /**
   * Creates a new handle from a vtkcamera and
   * insert it in the handles collection.
   */
  void InsertCamera(vtkCamera* camera, int index);

  /**
   * Create/Recreate npts default camera handles.
   */
  void CreateDefaultHandles(int npts);

  /**
   * Recreate the handles according to a
   * number of points equal to newNPts.
   * The old number of handles must be entered
   * as oldNPts.
   * It uses the current spline to recompute
   * the positions of the new handles.
   */
  void ReconfigureHandles(int newNPts, int oldNPts);

  /**
   * Specialized method to insert a camera handle on the camera path.
   */
  int InsertHandleOnLine(double* pos) override;

  /**
   * Specialized method to erase a camera handle from the camera path.
   */
  void EraseHandle(const int&) override;

  // Specialized methods to access handles
  vtkActor* GetHandleActor(int index) override;
  vtkHandleSource* GetHandleSource(int index) override;
  int GetHandleIndex(vtkProp* prop) override;

private:
  vtkCameraPathRepresentation(const vtkCameraPathRepresentation&) = delete;
  void operator=(const vtkCameraPathRepresentation&) = delete;

  void RebuildRepresentation();

  void UpdateConfiguration(int npts);

  std::vector<vtkSmartPointer<vtkCameraHandleSource>> CameraHandles;
  std::vector<vtkSmartPointer<vtkActor>> HandleActors;
};

VTK_ABI_NAMESPACE_END
#endif
