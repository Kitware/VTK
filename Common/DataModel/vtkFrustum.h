// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkFrustum
 * @brief   implicit function for a frustum
 *
 * vtkFrustum represents a 4-sided frustum, with a near plane but infinite on the far side. It is
 * defined by the two angles between its forward axis and its horizontal and vertical planes, and
 * the distance between its origin and near plane. vtkFrustum is a concrete implementation of
 * vtkImplicitFunction. The frustum is oriented toward the Y Axis; its top face facing
 * toward the Z Axis and its "right" face facing the X Axis.
 *
 * @warning
 * The frustum is infinite in extent towards its far plane. To truncate the frustum in modeling
 * operations use the vtkImplicitBoolean in combination with clipping planes.
 *
 */

#ifndef vtkFrustum_h
#define vtkFrustum_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"
#include "vtkNew.h" // For vtkNew

VTK_ABI_NAMESPACE_BEGIN
class vtkImplicitBoolean;
class vtkPlane;

class VTKCOMMONDATAMODEL_EXPORT vtkFrustum : public vtkImplicitFunction
{
public:
  static vtkFrustum* New();
  vtkTypeMacro(vtkFrustum, vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;

  using vtkImplicitFunction::EvaluateGradient;
  void EvaluateGradient(double x[3], double g[3]) override;

  ///@{
  /**
   * Get/Set the near plane distance of the frustum, i.e. the distance between its origin and near
   * plane along the forward axis. Values below 0 will be clamped. Defaults to 0.5.
   */
  vtkGetMacro(NearPlaneDistance, double)
  void SetNearPlaneDistance(double distance);
  ///@}

  ///@{
  /**
   * Get/Set the horizontal angle of the frustum in degrees. It represents the angle between its
   * forward axis and its right and left planes. Clamped between 1 and 89 degrees. Defaults to 30.
   */
  vtkGetMacro(HorizontalAngle, double)
  void SetHorizontalAngle(double angleInDegrees);
  ///@}

  ///@{
  /**
   * Get/Set the vertical angle of the frustum in degrees. It represents the angle between its
   * forward axis and its top and bottom planes. Clamped between 1 and 89 degrees. Defaults to 30.
   */
  vtkGetMacro(VerticalAngle, double)
  void SetVerticalAngle(double angleInDegrees);
  ///@}

  ///@{
  /**
   * Get individual planes that make up the frustum.
   * @note: Do not attempt to modify ! Use the vertical/horizontal angles and near plane distance to
   * parameterize the frustum instead.
   */
  vtkPlane* GetTopPlane() { return this->TopPlane; }
  vtkPlane* GetBottomPlane() { return this->BottomPlane; }
  vtkPlane* GetRightPlane() { return this->RightPlane; }
  vtkPlane* GetLeftPlane() { return this->LeftPlane; }
  vtkPlane* GetNearPlane() { return this->NearPlane; }
  ///@}

protected:
  vtkFrustum();
  ~vtkFrustum() override;

private:
  vtkFrustum(const vtkFrustum&) = delete;
  void operator=(const vtkFrustum&) = delete;

  ///@{
  /**
   * Compute and set the horizontal or vertical planes' normals according to the defined angle
   * Normals are pointing "outside" the frustum
   * @see vtkImplicitFunction and EvaluateFunction
   */
  void CalculateHorizontalPlanesNormal();
  void CalculateVerticalPlanesNormal();
  ///@}

  double NearPlaneDistance = 0.5;
  double VerticalAngle = 30;
  double HorizontalAngle = 30;

  vtkNew<vtkPlane> NearPlane;
  vtkNew<vtkPlane> BottomPlane;
  vtkNew<vtkPlane> TopPlane;
  vtkNew<vtkPlane> RightPlane;
  vtkNew<vtkPlane> LeftPlane;

  vtkNew<vtkImplicitBoolean> BooleanOp;
};

VTK_ABI_NAMESPACE_END
#endif
