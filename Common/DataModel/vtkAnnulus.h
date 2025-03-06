// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkAnnulus
 * @brief   implicit function for a annulus
 *
 * vtkAnnulus computes the implicit function and function gradient
 * for an annulus composed of two co-axial cylinders. vtkAnnulus is a concrete
 * implementation of vtkImplicitFunction. By default the Annulus is
 * centered at the origin and the axis of rotation is along the
 * y-axis. You can redefine the center and axis of rotation by setting
 * the Center and Axis data members. (Note that it is also possible to
 * use the superclass' vtkImplicitFunction transformation matrix if
 * necessary to reposition by using FunctionValue() and
 * FunctionGradient().)
 *
 * @warning
 * The annulus is infinite in extent. To truncate the annulus in
 * modeling operations use the vtkImplicitBoolean in combination with
 * clipping planes.
 *
 */

#ifndef vtkAnnulus_h
#define vtkAnnulus_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"
#include "vtkNew.h"    // For vtkNew
#include "vtkVector.h" // For vtkVector3d

VTK_ABI_NAMESPACE_BEGIN
class vtkCylinder;
class vtkImplicitBoolean;

class VTKCOMMONDATAMODEL_EXPORT vtkAnnulus : public vtkImplicitFunction
{
public:
  static vtkAnnulus* New();
  vtkTypeMacro(vtkAnnulus, vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Evaluate annulus equation.
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;
  ///@}

  /**
   * Evaluate annulus function gradient.
   */
  void EvaluateGradient(double x[3], double g[3]) override;

  ///@{
  /**
   * Set/Get the inner annulus radius. Default is 0.25.
   */
  void SetInnerRadius(double radius);
  double GetInnerRadius() const;
  ///@}

  ///@{
  /**
   * Set/Get the outer annulus radius. Default is 0.5.
   */
  void SetOuterRadius(double radius);
  double GetOuterRadius() const;
  ///@}

  ///@{
  /**
   * Set/Get the annulus center. Default is (0, 0, 0).
   */
  void SetCenter(double x, double y, double z);
  void SetCenter(const double xyz[3]);
  void SetCenter(const vtkVector3d& xyz);
  void GetCenter(double& x, double& y, double& z);
  void GetCenter(double xyz[3]);
  double* GetCenter() VTK_SIZEHINT(3);
  ///@}

  ///@{
  /**
   * Set/Get the axis of the annulus. If the axis is not specified as
   * a unit vector, it will be normalized. If zero-length axis vector
   * is used as input to this method, it will be ignored.
   * Default is the Y-axis (0, 1, 0)
   */
  void SetAxis(double x, double y, double z);
  void SetAxis(double axis[3]);
  void SetAxis(const vtkVector3d& axis);
  void GetAxis(double& x, double& y, double& z);
  void GetAxis(double xyz[3]);
  double* GetAxis() VTK_SIZEHINT(3);
  ///@}

protected:
  vtkAnnulus();
  ~vtkAnnulus() override;

private:
  vtkAnnulus(const vtkAnnulus&) = delete;
  void operator=(const vtkAnnulus&) = delete;

  void UpdateTransform();

  vtkVector3d Center = { 0.0, 0.0, 0.0 };
  vtkVector3d Axis = { 0.0, 1.0, 0.0 };

  vtkNew<vtkCylinder> InnerCylinder;
  vtkNew<vtkCylinder> OuterCylinder;
  vtkNew<vtkImplicitBoolean> BooleanOp;
};

VTK_ABI_NAMESPACE_END
#endif
