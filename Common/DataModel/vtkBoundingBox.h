/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkBoundingBox.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBoundingBox
 * @brief   Fast Simple Class for dealing with 3D bounds
 *
 * vtkBoundingBox maintains a 3D axis aligned bounding box.  It is very light
 * weight and many of the member functions are in-lined so it is very fast.
 * It is not derived from vtkObject so it can be allocated on the stack
 *
 * @sa
 * vtkBox
*/

#ifndef vtkBoundingBox_h
#define vtkBoundingBox_h
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSystemIncludes.h"

class VTKCOMMONDATAMODEL_EXPORT vtkBoundingBox
{
public:
  //@{
  /**
   * Construct a bounding box with the min point set to
   * VTK_DOUBLE_MAX and the max point set to VTK_DOUBLE_MIN.
   */
  vtkBoundingBox();
  vtkBoundingBox(const double bounds[6]);
  vtkBoundingBox(double xMin, double xMax,
                 double yMin, double yMax,
                 double zMin, double zMax);
  //@}

  /**
   * Copy constructor.
   */
  vtkBoundingBox(const vtkBoundingBox &bbox);

  /**
   * Assignment Operator
   */
  vtkBoundingBox &operator=(const vtkBoundingBox &bbox);

  //@{
  /**
   * Equality operator.
   */
  bool operator==(const vtkBoundingBox &bbox)const;
  bool operator!=(const vtkBoundingBox &bbox)const;
  //@}

  //@{
  /**
   * Set the bounds explicitly of the box (using the VTK convention for
   * representing a bounding box).  Returns 1 if the box was changed else 0.
   */
  void SetBounds(const double bounds[6]);
  void SetBounds(double xMin, double xMax,
                 double yMin, double yMax,
                 double zMin, double zMax);
  //@}

  //@{
  /**
   * Set the minimum point of the bounding box - if the min point
   * is greater than the max point then the max point will also be changed.
   */
  void SetMinPoint(double x, double y, double z);
  void SetMinPoint(double p[3]);
  //@}

  //@{
  /**
   * Set the maximum point of the bounding box - if the max point
   * is less than the min point then the min point will also be changed.
   */
  void SetMaxPoint(double x, double y, double z);
  void SetMaxPoint(double p[3]);
  //@}

  //@{
  /**
   * Returns 1 if the bounds have been set and 0 if the box is in its
   * initialized state which is an inverted state.
   */
  int IsValid() const;
  static int IsValid(const double bounds[6]);
  //@}

  //@{
  /**
   * Change bounding box so it includes the point p.  Note that the bounding
   * box may have 0 volume if its bounds were just initialized.
   */
  void AddPoint(double p[3]);
  void AddPoint(double px, double py, double pz);
  //@}

  /**
   * Change the bounding box to be the union of itself and the specified
   * bbox.
   */
  void AddBox(const vtkBoundingBox &bbox);

  /**
   * Adjust the bounding box so it contains the specified bounds (defined by
   * the VTK representation (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  void AddBounds(const double bounds[]);

  /**
   * Intersect this box with bbox. The method returns 1 if both boxes are
   * valid and they do have overlap else it will return 0.  If 0 is returned
   * the box has not been modified.
   */
  int IntersectBox(const vtkBoundingBox &bbox);

  /**
   * Returns 1 if the boxes intersect else returns 0.
   */
  int Intersects(const vtkBoundingBox &bbox) const;

  /**
   * Intersect this box with the half space defined by plane.  Returns true
   * if there is intersection---which implies that the box has been modified
   * Returns false otherwise.
   */
  bool IntersectPlane(double origin[3],double normal[3]);

  /**
   * Returns 1 if the min and max points of bbox are contained
   * within the bounds of the specified box, else returns 0.
   */
  int Contains(const vtkBoundingBox &bbox) const;

  //@{
  /**
   * Get the bounds of the box (defined by VTK style).
   */
  void GetBounds(double bounds[6]) const;
  void GetBounds(double &xMin, double &xMax,
                 double &yMin, double &yMax,
                 double &zMin, double &zMax) const;
  //@}

  /**
   * Return the ith bounds of the box (defined by VTK style).
   */
  double GetBound(int i) const;

  //@{
  /**
   * Get the minimum point of the bounding box.
   */
  const double *GetMinPoint() const VTK_SIZEHINT(3);
  void GetMinPoint(double &x, double &y, double &z) const;
  void GetMinPoint(double x[3]);
  //@}

  //@{
  /**
   * Get the maximum point of the bounding box.
   */
  const double *GetMaxPoint() const VTK_SIZEHINT(3);
  void GetMaxPoint(double &x, double &y, double &z) const;
  void GetMaxPoint(double x[3]);
  //@}

  /**
   * Get the ith corner of the bounding box. The points are ordered
   * with i, then j, then k increasing.
   */
  void GetCorner(int corner, double p[3]) const;

  //@{
  /**
   * Returns 1 if the point is contained in the box else 0.
   */
  vtkTypeBool ContainsPoint(double p[3]) const;
  vtkTypeBool ContainsPoint(double px, double py, double pz) const;
  //@}

  /**
   * Get the center of the bounding box.
   */
  void GetCenter(double center[3]) const;

  /**
   * Get the length of each sode of the box.
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

  /**
   * Return the length of the diagonal.
   * \pre not_empty: this->IsValid()
   */
  double GetDiagonalLength() const;

  //@{
  /**
   * Expand the bounding box. Inflate(delta) expands by delta on each side,
   * the box will grow by 2*delta in x, y, and z. Inflate(dx,dy,dz) expands
   * by the given amounts in each of the x, y, z directions. Finally,
   * Inflate() expands the bounds so that it has non-zero volume. Edges that
   * are inflated are adjusted 1% of the longest edge. Or if an edge is
   * zero length, the bounding box is inflated by 1 unit in that direction.
   */
  void Inflate(double delta);
  void Inflate(double deltaX, double deltaY, double deltaZ);
  void Inflate();
  //@}

  //@{
  /**
   * Scale each dimension of the box by some given factor.
   * If the box is not valid, it stays unchanged.
   * If the scalar factor is negative, bounds are flipped: for example,
   * if (xMin,xMax)=(-2,4) and sx=-3, (xMin,xMax) becomes (-12,6).
   */
  void Scale(double s[3]);
  void Scale(double sx, double sy, double sz);
  //@}

  //@{
  /**
   * Scale each dimension of the box by some given factor, with the origin of
   * the bounding box the center of the scaling. If the box is not
   * valid, it is not changed.
   */
  void ScaleAboutCenter(double s);
  void ScaleAboutCenter(double s[3]);
  void ScaleAboutCenter(double sx, double sy, double sz);
  //@}

  /**
   * Compute the number of divisions in the z-y-z directions given a target
   * number of total bins (i.e., product of divisions in the x-y-z
   * directions). The computation is done in such a way as to create near
   * cuboid bins. Also note that the returned bounds may be different than
   * the bounds defined in this class, as the bounds in the z-y-z directions
   * can never be <= 0. Note that the total number of divisions
   * (divs[0]*divs[1]*divs[2]) is guaranteed to be smaller than the target
   * number of bins.
   */
  vtkIdType ComputeDivisions(vtkIdType totalBins, double bounds[6], int divs[3]) const;

  /**
   * Returns the box to its initialized state.
   */
  void Reset();

protected:
  double MinPnt[3], MaxPnt[3];
};

inline void vtkBoundingBox::Reset()
{
  this->MinPnt[0] = this->MinPnt[1] = this->MinPnt[2] = VTK_DOUBLE_MAX;
  this->MaxPnt[0] = this->MaxPnt[1] = this->MaxPnt[2] = VTK_DOUBLE_MIN;
}

inline void vtkBoundingBox::GetBounds(double &xMin, double &xMax,
                                      double &yMin, double &yMax,
                                      double &zMin, double &zMax) const
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
  return ((i & 0x1) ? this->MaxPnt[i>>1] : this->MinPnt[i>>1]);
}

inline const double *vtkBoundingBox::GetMinPoint() const
{
  return this->MinPnt;
}

inline void vtkBoundingBox::GetMinPoint(double x[3])
{
  x[0] = this->MinPnt[0];
  x[1] = this->MinPnt[1];
  x[2] = this->MinPnt[2];
}

inline const double *vtkBoundingBox::GetMaxPoint() const
{
  return this->MaxPnt;
}

inline void vtkBoundingBox::GetMaxPoint(double x[3])
{
  x[0] = this->MaxPnt[0];
  x[1] = this->MaxPnt[1];
  x[2] = this->MaxPnt[2];
}

inline int vtkBoundingBox::IsValid() const
{
  return ((this->MinPnt[0] <= this->MaxPnt[0]) &&
          (this->MinPnt[1] <= this->MaxPnt[1]) &&
          (this->MinPnt[2] <= this->MaxPnt[2]));
}

inline int vtkBoundingBox::IsValid(const double bounds[6])
{
  return (bounds[0] <= bounds[1] &&
    bounds[2] <= bounds[3] &&
    bounds[4] <= bounds[5]);
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

inline void vtkBoundingBox::SetBounds(const double bounds[6])
{
  this->SetBounds(bounds[0], bounds[1], bounds[2],
                  bounds[3], bounds[4], bounds[5]);
}

inline void vtkBoundingBox::GetBounds(double bounds[6]) const
{
  this->GetBounds(bounds[0], bounds[1], bounds[2],
                  bounds[3], bounds[4], bounds[5]);
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

inline vtkBoundingBox::vtkBoundingBox(double xMin, double xMax,
                                      double yMin, double yMax,
                                      double zMin, double zMax)
{
  this->Reset();
  this->SetBounds(xMin, xMax, yMin, yMax, zMin, zMax);
}

inline vtkBoundingBox::vtkBoundingBox(const vtkBoundingBox &bbox)
{
  this->MinPnt[0] = bbox.MinPnt[0];
  this->MinPnt[1] = bbox.MinPnt[1];
  this->MinPnt[2] = bbox.MinPnt[2];

  this->MaxPnt[0] = bbox.MaxPnt[0];
  this->MaxPnt[1] = bbox.MaxPnt[1];
  this->MaxPnt[2] = bbox.MaxPnt[2];
}

inline vtkBoundingBox &vtkBoundingBox::operator=(const vtkBoundingBox &bbox)
{
  this->MinPnt[0] = bbox.MinPnt[0];
  this->MinPnt[1] = bbox.MinPnt[1];
  this->MinPnt[2] = bbox.MinPnt[2];

  this->MaxPnt[0] = bbox.MaxPnt[0];
  this->MaxPnt[1] = bbox.MaxPnt[1];
  this->MaxPnt[2] = bbox.MaxPnt[2];
  return *this;
}

inline bool vtkBoundingBox::operator==(const vtkBoundingBox &bbox)const
{
  return ((this->MinPnt[0] == bbox.MinPnt[0]) &&
          (this->MinPnt[1] == bbox.MinPnt[1]) &&
          (this->MinPnt[2] == bbox.MinPnt[2]) &&
          (this->MaxPnt[0] == bbox.MaxPnt[0]) &&
          (this->MaxPnt[1] == bbox.MaxPnt[1]) &&
          (this->MaxPnt[2] == bbox.MaxPnt[2]));
}

inline bool vtkBoundingBox::operator!=(const vtkBoundingBox &bbox)const
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

inline void vtkBoundingBox::GetMinPoint(double &x, double &y, double &z) const
{
  x = this->MinPnt[0];
  y = this->MinPnt[1];
  z = this->MinPnt[2];
}

inline void vtkBoundingBox::GetMaxPoint(double &x, double &y, double &z) const
{
  x = this->MaxPnt[0];
  y = this->MaxPnt[1];
  z = this->MaxPnt[2];
}

inline vtkTypeBool vtkBoundingBox::ContainsPoint(double px, double py,
                                         double pz) const
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

inline vtkTypeBool vtkBoundingBox::ContainsPoint(double p[3]) const
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

  int ix = (corner & 1)        ? 1 : 0;  // 0,1,0,1,0,1,0,1
  int iy = ((corner >> 1) & 1) ? 1 : 0;  // 0,0,1,1,0,0,1,1
  int iz = (corner >> 2)       ? 1 : 0;  // 0,0,0,0,1,1,1,1

  const double* pts[2] = { this->MinPnt, this->MaxPnt };
  p[0] = pts[ix][0];
  p[1] = pts[iy][1];
  p[2] = pts[iz][2];
}

#endif
// VTK-HeaderTest-Exclude: vtkBoundingBox.h
