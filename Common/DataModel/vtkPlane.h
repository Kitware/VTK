/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlane.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPlane
 * @brief   perform various plane computations
 *
 * vtkPlane provides methods for various plane computations. These include
 * projecting points onto a plane, evaluating the plane equation, and
 * returning plane normal. vtkPlane is a concrete implementation of the
 * abstract class vtkImplicitFunction.
*/

#ifndef vtkPlane_h
#define vtkPlane_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class VTKCOMMONDATAMODEL_EXPORT vtkPlane : public vtkImplicitFunction
{
public:
  /**
   * Construct plane passing through origin and normal to z-axis.
   */
  static vtkPlane *New();

  vtkTypeMacro(vtkPlane,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Evaluate plane equation for point x[3].
   */
  using vtkImplicitFunction::EvaluateFunction;
  void EvaluateFunction(vtkDataArray* input, vtkDataArray* output) override;
  double EvaluateFunction(double x[3]) override;
  //@}

  /**
   * Evaluate function gradient at point x[3].
   */
  void EvaluateGradient(double x[3], double g[3]) override;

  //@{
  /**
   * Set/get plane normal. Plane is defined by point and normal.
   */
  vtkSetVector3Macro(Normal,double);
  vtkGetVectorMacro(Normal,double,3);
  //@}

  //@{
  /**
   * Set/get point through which plane passes. Plane is defined by point
   * and normal.
   */
  vtkSetVector3Macro(Origin,double);
  vtkGetVectorMacro(Origin,double,3);
  //@}

  /**
   * Translate the plane in the direction of the normal by the
   * distance specified.  Negative values move the plane in the
   * opposite direction.
   */
  void Push(double distance);

  //@{
  /**
   * Project a point x onto plane defined by origin and normal. The
   * projected point is returned in xproj. NOTE : normal assumed to
   * have magnitude 1.
   */
  static void ProjectPoint(const double x[3], const double origin[3], const double normal[3],
                           double xproj[3]);
  void ProjectPoint(const double x[3], double xproj[3]);
  //@}

  //@{
  /**
   * Project a vector v onto plane defined by origin and normal. The
   * projected vector is returned in vproj.
   */
  static void ProjectVector(const double v[3], const double origin[3], const double normal[3],
                           double vproj[3]);
  void ProjectVector(const double v[3], double vproj[3]);
  //@}

  //@{
  /**
   * Project a point x onto plane defined by origin and normal. The
   * projected point is returned in xproj. NOTE : normal does NOT have to
   * have magnitude 1.
   */
  static void GeneralizedProjectPoint(const double x[3], const double origin[3],
                                      const double normal[3], double xproj[3]);
  void GeneralizedProjectPoint(const double x[3], double xproj[3]);
  //@}


  /**
   * Quick evaluation of plane equation n(x-origin)=0.
   */
  static double Evaluate(double normal[3], double origin[3], double x[3]);

  //@{
  /**
   * Return the distance of a point x to a plane defined by n(x-p0) = 0. The
   * normal n[3] must be magnitude=1.
   */
  static double DistanceToPlane(double x[3], double n[3], double p0[3]);
  double DistanceToPlane(double x[3]);
  //@}

  //@{
  /**
   * Given a line defined by the two points p1,p2; and a plane defined by the
   * normal n and point p0, compute an intersection. The parametric
   * coordinate along the line is returned in t, and the coordinates of
   * intersection are returned in x. A zero is returned if the plane and line
   * do not intersect between (0<=t<=1). If the plane and line are parallel,
   * zero is returned and t is set to VTK_LARGE_DOUBLE.
   */
  static int IntersectWithLine(const double p1[3], const double p2[3], double n[3],
                               double p0[3], double& t, double x[3]);
  int IntersectWithLine(const double p1[3], const double p2[3], double& t, double x[3]);
  //@}

  //@{
  /**
   * Given two planes, one infinite and one finite, defined by the normal n
   * and point o (infinite plane), and the second finite plane1 defined by
   * the three points (pOrigin,px,py), compute a line of intersection (if
   * any). The line of intersection is defined by the return values
   * (x0,x1). If there is no intersection, then zero is returned; otherwise
   * non-zero. There are two variants of this method. The static function
   * operates on the supplied function parameters; the non-static operates on
   * this instance of vtkPlane (and its associated origin and normal).
   */
  static int IntersectWithFinitePlane(double n[3], double o[3],
                                      double pOrigin[3], double px[3], double py[3],
                                      double x0[3], double x1[3]);
  int IntersectWithFinitePlane(double pOrigin[3], double px[3], double py[3],
                               double x0[3], double x1[3]);
  //@}

protected:
  vtkPlane();
  ~vtkPlane() override {}

  double Normal[3];
  double Origin[3];

private:
  vtkPlane(const vtkPlane&) = delete;
  void operator=(const vtkPlane&) = delete;
};

// Generally the normal should be normalized
inline double vtkPlane::Evaluate(double normal[3],
                                 double origin[3], double x[3])
{
  return normal[0]*(x[0]-origin[0]) + normal[1]*(x[1]-origin[1]) +
         normal[2]*(x[2]-origin[2]);
}

// Assumes normal is normalized
inline double vtkPlane::DistanceToPlane(double x[3], double n[3], double p0[3])
{
#define vtkPlaneAbs(x) ((x)<0?-(x):(x))
  return (vtkPlaneAbs(n[0]*(x[0]-p0[0]) + n[1]*(x[1]-p0[1]) +
                      n[2]*(x[2]-p0[2])));
}

#endif
