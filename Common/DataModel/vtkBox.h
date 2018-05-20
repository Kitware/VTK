/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBox.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBox
 * @brief   implicit function for a bounding box
 *
 * vtkBox computes the implicit function and/or gradient for a axis-aligned
 * bounding box. (The superclasses transform can be used to modify this
 * orientation.) Each side of the box is orthogonal to all other sides
 * meeting along shared edges and all faces are orthogonal to the x-y-z
 * coordinate axes.  (If you wish to orient this box differently, recall that
 * the superclass vtkImplicitFunction supports a transformation matrix.)
 * vtkBox is a concrete implementation of vtkImplicitFunction.
 *
 * @sa
 * vtkCubeSource vtkImplicitFunction
*/

#ifndef vtkBox_h
#define vtkBox_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"
class vtkBoundingBox;

class VTKCOMMONDATAMODEL_EXPORT vtkBox : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkBox,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct box with center at (0,0,0) and each side of length 1.0.
   */
  static vtkBox *New();

  /**
   * Evaluate box defined by the two points (pMin,pMax).
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;

  /**
   * Evaluate the gradient of the box.
   */
  void EvaluateGradient(double x[3], double n[3]) override;

  //@{
  /**
   * Set / get the bounding box using various methods.
   */
  void SetXMin(double p[3]);
  void SetXMin(double x, double y, double z);
  void GetXMin(double p[3]);
  void GetXMin(double &x, double &y, double &z);
  //@}

  void SetXMax(double p[3]);
  void SetXMax(double x, double y, double z);
  void GetXMax(double p[3]);
  void GetXMax(double &x, double &y, double &z);

  void SetBounds(double xMin, double xMax,
                 double yMin, double yMax,
                 double zMin, double zMax);
  void SetBounds(const double bounds[6]);
  void GetBounds(double &xMin, double &xMax,
                 double &yMin, double &yMax,
                 double &zMin, double &zMax);
  void GetBounds(double bounds[6]);
  double *GetBounds() VTK_SIZEHINT(6);

  /**
   * A special method that allows union set operation on bounding boxes.
   * Start with a SetBounds(). Subsequent AddBounds() methods are union set
   * operations on the original bounds. Retrieve the final bounds with a
   * GetBounds() method.
   */
  void AddBounds(const double bounds[6]);

  /**
   * Bounding box intersection with line modified from Graphics Gems Vol
   * I. The method returns a non-zero value if the bounding box is
   * hit. Origin[3] starts the ray, dir[3] is the vector components of the
   * ray in the x-y-z directions, coord[3] is the location of hit, and t is
   * the parametric coordinate along line. (Notes: the intersection ray
   * dir[3] is NOT normalized.  Valid intersections will only occur between
   * 0<=t<=1.)
   */
  static char IntersectBox(double bounds[6], const double origin[3], double dir[3],
                           double coord[3], double& t);

  /**
   * Intersect a line with the box.  Give the endpoints of the line in
   * p1 and p2.  The parameteric distances from p1 to the entry and exit
   * points are returned in t1 and t2, where t1 and t2 are clamped to the
   * range [0,1].  The entry and exit planes are returned in plane1 and
   * plane2 where integers (0, 1, 2, 3, 4, 5) stand for the
   * (xmin, xmax, ymin, ymax, zmin, zmax) planes respectively, and a value
   * of -1 means that no intersection occurred.  The actual intersection
   * coordinates are stored in x1 and x2, which can be set to nullptr of you
   * do not need them to be returned.  The function return value will be
   * zero if the line is wholly outside of the box.
   */
  static int IntersectWithLine(const double bounds[6],
                               const double p1[3], const double p2[3],
                               double &t1, double &t2,
                               double x1[3], double x2[3],
                               int &plane1, int &plane2);

  /**
   * Plane intersection with the box. The plane is infinite in extent and
   * defined by an origin and normal. The function indicates whether the
   * plane intersects, not the particulars of intersection points and such.
   * The function returns non-zero if the plane and box intersect; zero
   * otherwise.
   */
  static vtkTypeBool IntersectWithPlane(double bounds[6], double origin[3],
                                double normal[3]);

  /**
   * Plane intersection with the box. The plane is infinite in extent and
   * defined by an origin and normal. The function returns the number of
   * intersection points, and if does, up to six ordered intersection points
   * are provided (i.e., the points are ordered and form a valid polygon).
   * Thus the function returns non-zero if the plane and box intersect; zero
   * otherwise. Note that if there is an intersection, the number of
   * intersections ranges from [3,6]. xints memory layout is consistent with
   * vtkPoints array layout and is organized as (xyz, xyz, xyz, xyz, xyz,
   * xyz).
   */
  static vtkTypeBool IntersectWithPlane(double bounds[6], double origin[3],
                                double normal[3], double xints[18]);

protected:
  vtkBox();
  ~vtkBox() override;

  vtkBoundingBox *BBox;
  double Bounds[6]; //supports the GetBounds() method

private:
  vtkBox(const vtkBox&) = delete;
  void operator=(const vtkBox&) = delete;
};



inline void vtkBox::SetXMin(double p[3])
{
  this->SetXMin(p[0], p[1], p[2]);
}

inline void vtkBox::SetXMax(double p[3])
{
  this->SetXMax(p[0], p[1], p[2]);
}


#endif
