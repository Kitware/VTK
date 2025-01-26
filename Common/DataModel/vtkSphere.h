// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSphere
 * @brief   implicit function for a sphere
 *
 * vtkSphere computes the implicit function and/or gradient for a sphere.
 * vtkSphere is a concrete implementation of vtkImplicitFunction. Additional
 * methods are available for sphere-related computations, such as computing
 * bounding spheres for a set of points, or set of spheres.
 */

#ifndef vtkSphere_h
#define vtkSphere_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkSphere : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkSphere, vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct sphere with center at (0,0,0) and radius=0.5.
   */
  static vtkSphere* New();

  ///@{
  /**
   * Evaluate sphere equation ((x-x0)^2 + (y-y0)^2 + (z-z0)^2) - R^2.
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;
  ///@}

  /**
   * Evaluate sphere gradient.
   */
  void EvaluateGradient(double x[3], double n[3]) override;

  ///@{
  /**
   * Set / get the radius of the sphere. The default is 0.5.
   */
  vtkSetMacro(Radius, double);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Set / get the center of the sphere. The default is (0,0,0).
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVectorMacro(Center, double, 3);
  ///@}

  /**
   * Quick evaluation of the sphere equation ((x-x0)^2 + (y-y0)^2 + (z-z0)^2) - R^2.
   */
  static double Evaluate(double center[3], double R, double x[3])
  {
    return (x[0] - center[0]) * (x[0] - center[0]) + (x[1] - center[1]) * (x[1] - center[1]) +
      (x[2] - center[2]) * (x[2] - center[2]) - R * R;
  }

  ///@{
  /**
   * Create a bounding sphere from a set of points. The set of points is
   * defined by an array of doubles, in the order of x-y-z (which repeats for
   * each point).  An optional hints array provides a guess for the initial
   * bounding sphere; the two values in the hints array are the two points
   * expected to be the furthest apart. The output sphere consists of a
   * center (x-y-z) and a radius.
   */
  static void ComputeBoundingSphere(
    float* pts, vtkIdType numPts, float sphere[4], vtkIdType hints[2]);
  static void ComputeBoundingSphere(
    double* pts, vtkIdType numPts, double sphere[4], vtkIdType hints[2]);
  ///@}

  ///@{
  /**
   * Create a bounding sphere from a set of spheres. The set of input spheres
   * is defined by an array of pointers to spheres. Each sphere is defined by
   * the 4-tuple: center(x-y-z)+radius. An optional hints array provides a
   * guess for the initial bounding sphere; the two values in the hints array
   * are the two spheres expected to be the furthest apart. The output sphere
   * consists of a center (x-y-z) and a radius.
   */
  static void ComputeBoundingSphere(
    float** spheres, vtkIdType numSpheres, float sphere[4], vtkIdType hints[2]);
  static void ComputeBoundingSphere(
    double** spheres, vtkIdType numSpheres, double sphere[4], vtkIdType hints[2]);
  ///@}

  ///@{
  /**
   * Create a bounding sphere from a set of points. The set of points is
   * defined by an array of doubles or an array of floats, in the order of x-y-z
   * (which repeats for each point). The output sphere consists of a
   * center (x-y-z) and a radius.
   */
  static void ComputeBoundingSphere(double* pts, vtkIdType numPts, double sphere[4])
  {
    vtkSphere::ComputeBoundingSphere(pts, numPts, sphere, nullptr);
  }
  static void ComputeBoundingSphere(float* pts, vtkIdType numPts, float sphere[4])
  {
    vtkSphere::ComputeBoundingSphere(pts, numPts, sphere, nullptr);
  }
  ///@}

  ///@{
  /**
   * Create a bounding sphere from a set of spheres. The set of input spheres
   * is defined by an array of pointers to spheres. Each sphere is defined by
   * the 4-tuple: center(x-y-z)+radius. The output sphere consists of a
   * center (x-y-z) and a radius.
   */
  static void ComputeBoundingSphere(float** spheres, vtkIdType numSpheres, float sphere[4])
  {
    vtkSphere::ComputeBoundingSphere(spheres, numSpheres, sphere, nullptr);
  }
  static void ComputeBoundingSphere(double** spheres, vtkIdType numSpheres, double sphere[4])
  {
    vtkSphere::ComputeBoundingSphere(spheres, numSpheres, sphere, nullptr);
  }
  ///@}

protected:
  vtkSphere();
  ~vtkSphere() override = default;

  double Radius;
  double Center[3];

private:
  vtkSphere(const vtkSphere&) = delete;
  void operator=(const vtkSphere&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
