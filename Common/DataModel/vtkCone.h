// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkCone
 * @brief   implicit function for a cone
 *
 * vtkCone computes the implicit function and function gradient for a cone.
 * vtkCone is a concrete implementation of vtkImplicitFunction. By default, the cone vertex
 * is located at the origin with axis of rotation coincident with x-axis. You can use
 * the superclass' vtkImplicitFunction transformation matrix to reposition. You can alternatively
 * use the accessors provided by this class, which will cause the transform to be recomputed. to
 * reposition/orient the cone. The angle specifies the angle between the axis of rotation and the
 * side of the cone.
 *
 * @warning
 * The cone is infinite in extent (on both sides if IsDoubleCone is set to true). To truncate the
 * cone use the vtkImplicitBoolean in combination with clipping planes.
 */

#ifndef vtkCone_h
#define vtkCone_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkCone : public vtkImplicitFunction
{
public:
  /**
   * Construct cone with angle of 45 degrees.
   */
  static vtkCone* New();

  vtkTypeMacro(vtkCone, vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using vtkImplicitFunction::EvaluateFunction;
  ///@{
  /**
   * Evaluate cone equation.
   */
  double EvaluateFunction(double x[3]) override;
  ///@}

  /**
   * Evaluate cone normal.
   */
  void EvaluateGradient(double x[3], double g[3]) override;

  ///@{
  /**
   * Set/Get the cone angle (expressed in degrees).
   * Defaults to 45 degrees.
   */
  vtkSetClampMacro(Angle, double, 0.0, 89.0);
  vtkGetMacro(Angle, double);
  ///@}

  ///@{
  /**
   * Set/Get the cone origin.
   * Defaults to (0, 0, 0).
   */
  void SetOrigin(double x, double y, double z);
  void SetOrigin(const double xyz[3]);
  vtkGetVector3Macro(Origin, double);
  ///@}

  ///@{
  /**
   * Get/Set the vector defining the direction of the cone.
   * If the axis is not specified as
   * a unit vector, it will be normalized. If zero-length axis vector
   * is used as input to this method, it will be ignored.
   * Defaults to the X axis (1, 0, 0).
   */
  void SetAxis(double x, double y, double z);
  void SetAxis(double axis[3]);
  vtkGetVector3Macro(Axis, double);
  ///@}

  ///@{
  /**
   * Set/Get whether this is a double cone (extends to infinity on both directions along axis) or a
   * one sided one (extends towards the axis direction only).
   * vtkCone is a double cone by default.
   */
  vtkSetMacro(IsDoubleCone, bool);
  vtkGetMacro(IsDoubleCone, bool);
  vtkBooleanMacro(IsDoubleCone, bool);
  ///@}

protected:
  vtkCone() = default;
  ~vtkCone() override = default;

  double Angle = 45.0;
  double Origin[3] = { 0.0, 0.0, 0.0 };
  double Axis[3] = { 1.0, 0.0, 0.0 };
  bool IsDoubleCone = true;

private:
  vtkCone(const vtkCone&) = delete;
  void operator=(const vtkCone&) = delete;

  /**
   * @brief Compute the function's transform according to the currently set origin/axis.
   * Called after any modification to one of these attributes.
   */
  void UpdateTransform();
};

VTK_ABI_NAMESPACE_END
#endif
