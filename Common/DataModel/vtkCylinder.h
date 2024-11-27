// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCylinder
 * @brief   implicit function for a cylinder
 *
 * vtkCylinder computes the implicit function and function gradient
 * for a cylinder using F(r)=r^2-Radius^2. vtkCylinder is a concrete
 * implementation of vtkImplicitFunction. By default the Cylinder is
 * centered at the origin and the axis of rotation is along the
 * y-axis. You can redefine the center and axis of rotation by setting
 * the Center and Axis data members. (Note that it is also possible to
 * use the superclass' vtkImplicitFunction transformation matrix if
 * necessary to reposition by using FunctionValue() and
 * FunctionGradient().)
 *
 * @warning
 * The cylinder is infinite in extent. To truncate the cylinder in
 * modeling operations use the vtkImplicitBoolean in combination with
 * clipping planes.
 *
 * @sa
 * vtkCylinderSource
 */

#ifndef vtkCylinder_h
#define vtkCylinder_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkCylinder : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkCylinder, vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct cylinder radius of 0.5; centered at origin with axis
   * along y coordinate axis.
   */
  static vtkCylinder* New();

  ///@{
  /**
   * Evaluate cylinder equation F(r) = r^2 - Radius^2.
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;
  ///@}

  /**
   * Evaluate cylinder function gradient.
   */
  void EvaluateGradient(double x[3], double g[3]) override;

  ///@{
  /**
   * Set/Get the cylinder radius.
   */
  vtkSetClampMacro(Radius, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Set/Get the cylinder center.
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVector3Macro(Center, double);
  ///@}

  ///@{
  /**
   * Set/Get the axis of the cylinder. If the axis is not specified as
   * a unit vector, it will be normalized. If zero-length axis vector
   * is used as input to this method, it will be ignored.
   */
  void SetAxis(double ax, double ay, double az);
  void SetAxis(double a[3]);
  vtkGetVector3Macro(Axis, double);
  ///@}

protected:
  vtkCylinder();
  ~vtkCylinder() override = default;

  double Radius;
  double Center[3];
  double Axis[3];

private:
  vtkCylinder(const vtkCylinder&) = delete;
  void operator=(const vtkCylinder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
