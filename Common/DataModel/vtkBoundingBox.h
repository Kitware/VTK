// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBoundingBox
 * @brief   Fast, simple class for representing and operating on 3D bounds
 *
 * vtkBoundingBox maintains and performs operations on a 3D axis aligned
 * bounding box. It is very light weight and many of the member functions are
 * in-lined so it is very fast. It is not derived from vtkObject so it can
 * be allocated on the stack.
 *
 * @sa
 * vtkBox
 */

#ifndef vtkBoundingBox_h
#define vtkBoundingBox_h
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSystemIncludes.h"
#include <atomic> // For threaded bounding box computation

VTK_ABI_NAMESPACE_BEGIN
class vtkPoints;

class VTKCOMMONDATAMODEL_EXPORT vtkBoundingBox
{
public:
  ///@{
  /**
   * Construct a bounding box with the min point set to
   * VTK_DOUBLE_MAX and the max point set to VTK_DOUBLE_MIN.
   */
  vtkBoundingBox();
  /**
   * Construct a bounding box with given bounds.
   */
  vtkBoundingBox(const double bounds[6]);
  /**
   * Construct a bounding box with given bounds.
   */
  vtkBoundingBox(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);
  /**
   * Construct a bounding box around center, inflated by delta (so final length is 2*delta)
   */
  vtkBoundingBox(double center[3], double delta);
  ///@}

  /**
   * Copy constructor.
   */
  vtkBoundingBox(const vtkBoundingBox& bbox);

  /**
   * Assignment Operator
   */
  vtkBoundingBox& operator=(const vtkBoundingBox& bbox);

  ///@{
  /**
   * Equality operator.
   */
  bool operator==(const vtkBoundingBox& bbox) const;
  bool operator!=(const vtkBoundingBox& bbox) const;
  ///@}

  ///@{
  /**
   * Set the bounds explicitly of the box (using the VTK convention for
   * representing a bounding box).  Returns 1 if the box was changed else 0.
   */
  void SetBounds(const double bounds[6]);
  void SetBounds(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);
  ///@}

  ///@{
  /**
   * Compute the bounding box from an array of vtkPoints. It uses a fast
   * (i.e., threaded) path when possible. The second signature (with point
   * uses) only considers points with ptUses[i] != 0 in the bounds
   * calculation. The third signature uses point ids.
   * The non-static ComputeBounds() methods update the current bounds of an instance of this class.
   */
  static void ComputeBounds(vtkPoints* pts, double bounds[6]);
  static void ComputeBounds(vtkPoints* pts, const unsigned char* ptUses, double bounds[6]);
  static void ComputeBounds(
    vtkPoints* pts, const std::atomic<unsigned char>* ptUses, double bounds[6]);
  static void ComputeBounds(
    vtkPoints* pts, const long long* ptIds, long long numPointIds, double bounds[6]);
  static void ComputeBounds(vtkPoints* pts, const long* ptIds, long numPointIds, double bounds[6]);
  static void ComputeBounds(vtkPoints* pts, const int* ptIds, int numPointIds, double bounds[6]);
  void ComputeBounds(vtkPoints* pts)
  {
    double bds[6];
    vtkBoundingBox::ComputeBounds(pts, bds);
    this->MinPnt[0] = bds[0];
    this->MinPnt[1] = bds[2];
    this->MinPnt[2] = bds[4];
    this->MaxPnt[0] = bds[1];
    this->MaxPnt[1] = bds[3];
    this->MaxPnt[2] = bds[5];
  }
  void ComputeBounds(vtkPoints* pts, unsigned char* ptUses)
  {
    double bds[6];
    vtkBoundingBox::ComputeBounds(pts, ptUses, bds);
    this->MinPnt[0] = bds[0];
    this->MinPnt[1] = bds[2];
    this->MinPnt[2] = bds[4];
    this->MaxPnt[0] = bds[1];
    this->MaxPnt[1] = bds[3];
    this->MaxPnt[2] = bds[5];
  }
  ///@}

  ///@{
  /**
   * Compute local bounds.
   * Not as fast as vtkPoints.getBounds() if u, v, w form a natural basis.
   */
  static void ComputeLocalBounds(
    vtkPoints* points, double u[3], double v[3], double w[3], double outputBounds[6]);
  ///@}

  ///@{
  /**
   * Set the minimum point of the bounding box - if the min point
   * is greater than the max point then the max point will also be changed.
   */
  void SetMinPoint(double x, double y, double z);
  void SetMinPoint(double p[3]);
  ///@}

  ///@{
  /**
   * Set the maximum point of the bounding box - if the max point
   * is less than the min point then the min point will also be changed.
   */
  void SetMaxPoint(double x, double y, double z);
  void SetMaxPoint(double p[3]);
  ///@}

  ///@{
  /**
   * Returns 1 if the bounds have been set and 0 if the box is in its
   * initialized state which is an inverted state.
   */
  int IsValid() const;
  static int IsValid(const double bounds[6]);
  ///@}

  ///@{
  /**
   * Change bounding box so it includes the point p.  Note that the bounding
   * box may have 0 volume if its bounds were just initialized.
   */
  void AddPoint(double p[3]);
  void AddPoint(double px, double py, double pz);
  ///@}

  /**
   * Change the bounding box to be the union of itself and the specified
   * bbox.
   */
  void AddBox(const vtkBoundingBox& bbox);

  /**
   * Adjust the bounding box so it contains the specified bounds (defined by
   * the VTK representation (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  void AddBounds(const double bounds[6]);

  /**
   * Returns true if this instance is entirely contained by bbox.
   */
  bool IsSubsetOf(const vtkBoundingBox& bbox) const;

  /**
   * Intersect this box with bbox. The method returns 1 if both boxes are
   * valid and they do have overlap else it will return 0.  If 0 is returned
   * the box has not been modified.
   */
  int IntersectBox(const vtkBoundingBox& bbox);

  /**
   * Returns 1 if the boxes intersect else returns 0.
   */
  int Intersects(const vtkBoundingBox& bbox) const;

  /**
   * Intersect this box with the half space defined by plane.  Returns true
   * if there is intersection---which implies that the box has been modified
   * Returns false otherwise.
   */
  bool IntersectPlane(double origin[3], double normal[3]);

  /**
   * Intersect this box with a sphere.
   * Parameters involve the center of the sphere and the squared radius.
   */
  bool IntersectsSphere(double center[3], double squaredRadius) const;

  /**
   * Returns true if any part of segment [p1,p2] lies inside the bounding box, as well as on its
   * boundaries. It returns false otherwise.
   */
  bool IntersectsLine(const double p1[3], const double p2[3]) const;

  /**
   * Returns the inner dimension of the bounding box.
   */
  int ComputeInnerDimension() const;

  /**
   * Returns 1 if the min and max points of bbox are contained
   * within the bounds of the specified box, else returns 0.
   */
  int Contains(const vtkBoundingBox& bbox) const;

  /**
   * A specialized, performant method to compute the containment of a finite
   * line emanating from the center of a bounding box. The method returns
   * true if the box contains the line defined by (x,lineEnd); and false if
   * the line intersects the box (i.e., the line passes through the boundary
   * of the box). The box is defined by specifying a point at the center of
   * the box x[3] with sides lengths s[3] (in the x-y-z directions). If an
   * intersection occurs (i.e., the containment return value is false), then
   * the function returns the parametric coordinate of intersection t, the
   * position of intersection xInt[3], and the plane of intersection where
   * integers (0, 1, 2, 3, 4, 5) stand for the (xmin, xmax, ymin, ymax, zmin,
   * zmax) box planes respectively.  (If no intersection occurs, i.e., the
   * line is contained, then the line (x,lineEnd) is contained within the box
   * with x-y-z lengths s[3] centered around the point x, and the values of
   * t, xInt, and plane are undefined.)
   */
  static bool ContainsLine(const double x[3], const double s[3], const double lineEnd[3], double& t,
    double xInt[3], int& plane);

  ///@{
  /**
   * Get the bounds of the box (defined by VTK style).
   */
  void GetBounds(double bounds[6]) const;
  void GetBounds(
    double& xMin, double& xMax, double& yMin, double& yMax, double& zMin, double& zMax) const;
  ///@}

  /**
   * Return the ith bounds of the box (defined by VTK style).
   */
  double GetBound(int i) const;

  ///@{
  /**
   * Get the minimum point of the bounding box.
   */
  const double* GetMinPoint() const VTK_SIZEHINT(3);
  void GetMinPoint(double& x, double& y, double& z) const;
  void GetMinPoint(double x[3]) const;
  ///@}

  ///@{
  /**
   * Get the maximum point of the bounding box.
   */
  const double* GetMaxPoint() const VTK_SIZEHINT(3);
  void GetMaxPoint(double& x, double& y, double& z) const;
  void GetMaxPoint(double x[3]) const;
  ///@}

  /**
   * Get the ith corner of the bounding box. The points are ordered
   * with i, then j, then k increasing.
   */
  void GetCorner(int corner, double p[3]) const;

  ///@{
  /**
   * Returns 1 if the point is contained in the box else 0.
   */
  vtkTypeBool ContainsPoint(const double p[3]) const;
  vtkTypeBool ContainsPoint(double px, double py, double pz) const;
  template <class PointT>
  bool ContainsPoint(const PointT& p) const;
  ///@}

  /**
   * Get the center of the bounding box.
   */
  void GetCenter(double center[3]) const;

  /**
   * Get the length of each side of the box.
   */
  void GetLengths(double lengths[3]) const;

  /**
   * Return the length of the bounding box in the ith direction.
   */
  double GetLength(int i) const;

  /**
   * Return the maximum length of the box.
   */
  double GetMaxLength() const;

  ///@{
  /**
   * Return the length of the diagonal.
   * \pre not_empty: this->IsValid()
   */
  double GetDiagonalLength2() const;
  double GetDiagonalLength() const;
  ///@}

  ///@{
  /**
   * Expand the bounding box. Inflate(delta) expands by delta on each side,
   * the box will grow by 2*delta in x, y, and z. Inflate(dx,dy,dz) expands
   * by the given amounts in each of the x, y, z directions.  Inflate()
   * expands the bounds so that it has non-zero volume. Sides that are
   * inflated are adjusted by 1% of the longest edge. Or if an edge is zero
   * length, the bounding box is inflated by 1 unit in that direction.
   * Finally, InflateSlice(delta) will expand any side of the bounding box by
   * +/- delta if that side has length <2*delta (i.e., it is a slice as
   * measured by the user-specified delta)).
   */
  void Inflate(double delta);
  void Inflate(double deltaX, double deltaY, double deltaZ);
  void Inflate();
  void InflateSlice(double delta);
  ///@}

  ///@{
  /**
   * Scale each dimension of the box by some given factor.
   * If the box is not valid, it stays unchanged.
   * If the scalar factor is negative, bounds are flipped: for example,
   * if (xMin,xMax)=(-2,4) and sx=-3, (xMin,xMax) becomes (-12,6).
   */
  void Scale(double s[3]);
  void Scale(double sx, double sy, double sz);
  ///@}

  ///@{
  /**
   * Scale each dimension of the box by some given factor, with the origin of
   * the bounding box the center of the scaling. If the box is not
   * valid, it is not changed.
   */
  void ScaleAboutCenter(double s);
  void ScaleAboutCenter(double s[3]);
  void ScaleAboutCenter(double sx, double sy, double sz);
  ///@}

  /**
   * Compute the number of divisions in the x-y-z directions given a
   * psoitive, target number of total bins (i.e., product of divisions in the
   * x-y-z directions). The computation is done in such a way as to create
   * near cuboid bins. Also note that the returned bounds may be different
   * than the bounds defined in this class, as the bounds in the x-y-z
   * directions can never be <= 0. Note that the total number of divisions
   * (divs[0]*divs[1]*divs[2]) will be less than or equal to the target
   * number of bins (as long as totalBins>=1).
   */
  vtkIdType ComputeDivisions(vtkIdType totalBins, double bounds[6], int divs[3]) const;

  /**
   * Clamp the number of divisions to be less than or equal to a target number
   * of bins, and the divs[i] >= 1.
   */
  static void ClampDivisions(vtkIdType targetBins, int divs[3]);

  /**
   * Returns the box to its initialized state.
   */
  void Reset();

  /**
   * Clamp point so it is contained inside box.
   * Each coordinate is clamped with box bounds.
   */
  void ClampPoint(double point[3]);

  /**
   * For each axis, get the minimum distance to put the point inside the box.
   * A value of 0 means "between min and max" (for the given coordinates).
   * This is useful to get the minimum translation to apply to contains a point.
   * @see Translate.
   */
  void GetDistance(double point[3], double distance[3]);

  /**
   * Translate box from motion.
   * The value of motion is added to MinPoint and MaxPoint.
   */
  void Translate(double motion[3]);

protected:
  double MinPnt[3], MaxPnt[3];
};

inline void vtkBoundingBox::Reset()
{
  this->MinPnt[0] = this->MinPnt[1] = this->MinPnt[2] = VTK_DOUBLE_MAX;
  this->MaxPnt[0] = this->MaxPnt[1] = this->MaxPnt[2] = VTK_DOUBLE_MIN;
}

inline void vtkBoundingBox::GetBounds(
  double& xMin, double& xMax, double& yMin, double& yMax, double& zMin, double& zMax) const
{
  xMin = this->MinPnt[0];
  xMax = this->MaxPnt[0];
  yMin = this->MinPnt[1];
  yMax = this->MaxPnt[1];
  zMin = this->MinPnt[2];
  zMax = this->MaxPnt[2];
}

inline double vtkBoundingBox::GetBound(int i) const
{
  // If i is odd then when are returning a part of the max bounds
  // else part of the min bounds is requested.  The exact component
  // needed is i /2 (or i right shifted by 1
  return ((i & 0x1) ? this->MaxPnt[i >> 1] : this->MinPnt[i >> 1]);
}

inline const double* vtkBoundingBox::GetMinPoint() const
{
  return this->MinPnt;
}

inline void vtkBoundingBox::GetMinPoint(double x[3]) const
{
  x[0] = this->MinPnt[0];
  x[1] = this->MinPnt[1];
  x[2] = this->MinPnt[2];
}

inline const double* vtkBoundingBox::GetMaxPoint() const
{
  return this->MaxPnt;
}

inline void vtkBoundingBox::GetMaxPoint(double x[3]) const
{
  x[0] = this->MaxPnt[0];
  x[1] = this->MaxPnt[1];
  x[2] = this->MaxPnt[2];
}

inline int vtkBoundingBox::IsValid() const
{
  return ((this->MinPnt[0] <= this->MaxPnt[0]) && (this->MinPnt[1] <= this->MaxPnt[1]) &&
    (this->MinPnt[2] <= this->MaxPnt[2]));
}

inline int vtkBoundingBox::IsValid(const double bounds[6])
{
  return (bounds[0] <= bounds[1] && bounds[2] <= bounds[3] && bounds[4] <= bounds[5]);
}

inline double vtkBoundingBox::GetLength(int i) const
{
  return this->MaxPnt[i] - this->MinPnt[i];
}

inline void vtkBoundingBox::GetLengths(double lengths[3]) const
{
  lengths[0] = this->GetLength(0);
  lengths[1] = this->GetLength(1);
  lengths[2] = this->GetLength(2);
}

inline void vtkBoundingBox::GetCenter(double center[3]) const
{
  center[0] = 0.5 * (this->MaxPnt[0] + this->MinPnt[0]);
  center[1] = 0.5 * (this->MaxPnt[1] + this->MinPnt[1]);
  center[2] = 0.5 * (this->MaxPnt[2] + this->MinPnt[2]);
}

inline bool vtkBoundingBox::IsSubsetOf(const vtkBoundingBox& bbox) const
{
  const double* bboxMaxPnt = bbox.GetMaxPoint();
  const double* bboxMinPnt = bbox.GetMinPoint();
  return this->MaxPnt[0] < bboxMaxPnt[0] && this->MinPnt[0] > bboxMinPnt[0] &&
    this->MaxPnt[1] < bboxMaxPnt[1] && this->MinPnt[1] > bboxMinPnt[1] &&
    this->MaxPnt[2] < bboxMaxPnt[2] && this->MinPnt[2] > bboxMinPnt[2];
}

inline void vtkBoundingBox::SetBounds(const double bounds[6])
{
  this->SetBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

inline void vtkBoundingBox::GetBounds(double bounds[6]) const
{
  this->GetBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

inline vtkBoundingBox::vtkBoundingBox()
{
  this->Reset();
}

inline vtkBoundingBox::vtkBoundingBox(const double bounds[6])
{
  this->Reset();
  this->SetBounds(bounds);
}

inline vtkBoundingBox::vtkBoundingBox(
  double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{
  this->Reset();
  this->SetBounds(xMin, xMax, yMin, yMax, zMin, zMax);
}

inline vtkBoundingBox::vtkBoundingBox(const vtkBoundingBox& bbox)
{
  this->MinPnt[0] = bbox.MinPnt[0];
  this->MinPnt[1] = bbox.MinPnt[1];
  this->MinPnt[2] = bbox.MinPnt[2];

  this->MaxPnt[0] = bbox.MaxPnt[0];
  this->MaxPnt[1] = bbox.MaxPnt[1];
  this->MaxPnt[2] = bbox.MaxPnt[2];
}

inline vtkBoundingBox::vtkBoundingBox(double center[3], double delta)
{
  this->Reset();
  this->AddPoint(center);
  this->Inflate(delta);
}

inline vtkBoundingBox& vtkBoundingBox::operator=(const vtkBoundingBox& bbox)
{
  this->MinPnt[0] = bbox.MinPnt[0];
  this->MinPnt[1] = bbox.MinPnt[1];
  this->MinPnt[2] = bbox.MinPnt[2];

  this->MaxPnt[0] = bbox.MaxPnt[0];
  this->MaxPnt[1] = bbox.MaxPnt[1];
  this->MaxPnt[2] = bbox.MaxPnt[2];
  return *this;
}

inline bool vtkBoundingBox::operator==(const vtkBoundingBox& bbox) const
{
  return ((this->MinPnt[0] == bbox.MinPnt[0]) && (this->MinPnt[1] == bbox.MinPnt[1]) &&
    (this->MinPnt[2] == bbox.MinPnt[2]) && (this->MaxPnt[0] == bbox.MaxPnt[0]) &&
    (this->MaxPnt[1] == bbox.MaxPnt[1]) && (this->MaxPnt[2] == bbox.MaxPnt[2]));
}

inline bool vtkBoundingBox::operator!=(const vtkBoundingBox& bbox) const
{
  return !((*this) == bbox);
}

inline void vtkBoundingBox::SetMinPoint(double p[3])
{
  this->SetMinPoint(p[0], p[1], p[2]);
}

inline void vtkBoundingBox::SetMaxPoint(double p[3])
{
  this->SetMaxPoint(p[0], p[1], p[2]);
}

inline void vtkBoundingBox::GetMinPoint(double& x, double& y, double& z) const
{
  x = this->MinPnt[0];
  y = this->MinPnt[1];
  z = this->MinPnt[2];
}

inline void vtkBoundingBox::GetMaxPoint(double& x, double& y, double& z) const
{
  x = this->MaxPnt[0];
  y = this->MaxPnt[1];
  z = this->MaxPnt[2];
}

inline vtkTypeBool vtkBoundingBox::ContainsPoint(double px, double py, double pz) const
{
  if ((px < this->MinPnt[0]) || (px > this->MaxPnt[0]))
  {
    return 0;
  }
  if ((py < this->MinPnt[1]) || (py > this->MaxPnt[1]))
  {
    return 0;
  }
  if ((pz < this->MinPnt[2]) || (pz > this->MaxPnt[2]))
  {
    return 0;
  }
  return 1;
}

inline vtkTypeBool vtkBoundingBox::ContainsPoint(const double p[3]) const
{
  return this->ContainsPoint(p[0], p[1], p[2]);
}

template <class PointT>
inline bool vtkBoundingBox::ContainsPoint(const PointT& p) const
{
  return this->ContainsPoint(p[0], p[1], p[2]);
}

inline void vtkBoundingBox::GetCorner(int corner, double p[3]) const
{
  if ((corner < 0) || (corner > 7))
  {
    p[0] = VTK_DOUBLE_MAX;
    p[1] = VTK_DOUBLE_MAX;
    p[2] = VTK_DOUBLE_MAX;
    return; // out of bounds
  }

  int ix = (corner & 1) ? 1 : 0;        // 0,1,0,1,0,1,0,1
  int iy = ((corner >> 1) & 1) ? 1 : 0; // 0,0,1,1,0,0,1,1
  int iz = (corner >> 2) ? 1 : 0;       // 0,0,0,0,1,1,1,1

  const double* pts[2] = { this->MinPnt, this->MaxPnt };
  p[0] = pts[ix][0];
  p[1] = pts[iy][1];
  p[2] = pts[iz][2];
}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkBoundingBox.h
