// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSpheres
 * @brief   implicit function for a set of spheres
 *
 * vtkSpheres computes the implicit function and function gradient for a set
 * of spheres. The spheres are combined via a union operation (i.e., the
 * minimum value from the evaluation of all spheres is taken).
 *
 * The function value is the distance of a point to the closest sphere, with
 * negative values interior to the spheres, positive outside the spheres, and
 * distance=0 on the spheres surface.  The function gradient is the sphere
 * normal at the function value.
 *
 * @sa
 * vtkPlanes vtkImplicitBoolean
 */

#ifndef vtkSpheres_h
#define vtkSpheres_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkSphere;
class vtkPoints;
class vtkDataArray;

class VTKCOMMONDATAMODEL_EXPORT vtkSpheres : public vtkImplicitFunction
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkSpheres* New();
  vtkTypeMacro(vtkSpheres, vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Evaluate spheres equations. Return largest value when evaluating all
   * sphere equations.
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;
  ///@}

  /**
   * Evaluate spheres gradient. Gradients point towards the outside of the
   * spheres.
   */
  void EvaluateGradient(double x[3], double n[3]) override;

  ///@{
  /**
   * Specify a list of points defining sphere centers.
   */
  virtual void SetCenters(vtkPoints*);
  vtkGetObjectMacro(Centers, vtkPoints);
  ///@}

  ///@{
  /**
   * Specify a list of radii for the spheres. There is a one-to-one
   * correspondence between sphere points and sphere radii.
   */
  void SetRadii(vtkDataArray* radii);
  vtkGetObjectMacro(Radii, vtkDataArray);
  ///@}

  /**
   * Return the number of spheres in the set of spheres.
   */
  int GetNumberOfSpheres();

  /**
   * Create and return a pointer to a vtkSphere object at the ith
   * position. Asking for a sphere outside the allowable range returns
   * nullptr.  This method always returns the same object.  Alternatively use
   * GetSphere(int i, vtkSphere *sphere) to update a user supplied sphere.
   */
  vtkSphere* GetSphere(int i);

  /**
   * If i is within the allowable range, mutates the given sphere's
   * Center and Radius to match the vtkSphere object at the ith
   * position. Does nothing if i is outside the allowable range.
   */
  void GetSphere(int i, vtkSphere* sphere);

protected:
  vtkSpheres();
  ~vtkSpheres() override;

  vtkPoints* Centers;
  vtkDataArray* Radii;
  vtkSphere* Sphere;

private:
  vtkSpheres(const vtkSpheres&) = delete;
  void operator=(const vtkSpheres&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
