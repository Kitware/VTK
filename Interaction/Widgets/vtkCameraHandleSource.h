// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCameraHandleSource
 * @brief   handle source used to represent one camera.
 *
 * vtkCameraHandleSource is deriving vtkHandleSource interface.
 * This handle represents a camera pointing in the focal point direction.
 * Its position and size can be modified.
 * If the directional parameter is set to true,
 * the camera is represented by one big arrow in the focal point direction
 * and one smaller pointing in the view up direction.
 * Else, the camera is only displayed as a simple sphere.
 */

#ifndef vtkCameraHandleSource_h
#define vtkCameraHandleSource_h

#include "vtkCamera.h" // for vtkCamera
#include "vtkHandleSource.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkArrowSource;
class vtkActor;
class vtkAppendPolyData;
class vtkSphereSource;
class vtkTransform;
class vtkTransformFilter;

class VTKINTERACTIONWIDGETS_EXPORT vtkCameraHandleSource : public vtkHandleSource
{
public:
  static vtkCameraHandleSource* New();
  vtkTypeMacro(vtkCameraHandleSource, vtkHandleSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the camera represented by this handle.
   * The camera can't be set to nullptr.
   */
  void SetCamera(vtkCamera* cam);

  ///@{
  /**
   * Set/Get the position of the camera handle.
   */
  using vtkHandleSource::SetPosition;
  void SetPosition(double xPos, double yPos, double zPos) override;
  using vtkHandleSource::GetPosition;
  double* GetPosition() override;
  ///@}

  ///@{
  /**
   * Set/Get the direction of the camera handle.
   * The direction is used in case the camera handle is
   * represented as two arrows (when this->Directional = true).
   */
  using vtkHandleSource::SetDirection;
  void SetDirection(double xTarget, double yTarget, double zTarget) override;
  using vtkHandleSource::GetDirection;
  double* GetDirection() override;
  ///@}

  vtkCameraHandleSource(const vtkCameraHandleSource&) = delete;
  void operator=(const vtkCameraHandleSource&) = delete;

protected:
  vtkCameraHandleSource();
  ~vtkCameraHandleSource() override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void RecomputeArrows();

  void RecomputeSphere();

private:
  vtkSmartPointer<vtkCamera> Camera = vtkSmartPointer<vtkCamera>::New();
  vtkNew<vtkArrowSource> UpArrow;
  vtkNew<vtkArrowSource> FrontArrow;
  vtkNew<vtkTransform> UpTransform;
  vtkNew<vtkTransformFilter> UpTransformFilter;
  vtkNew<vtkTransform> FrontTransform;
  vtkNew<vtkTransformFilter> FrontTransformFilter;
  vtkNew<vtkAppendPolyData> ArrowsAppend;
  vtkNew<vtkSphereSource> PositionSphere;
};

VTK_ABI_NAMESPACE_END
#endif
