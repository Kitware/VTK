// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLine
 * @brief   cell represents a 1D line
 *
 * vtkLine is a concrete implementation of vtkCell to represent a 1D line.
 */

#ifndef vtkLine_h
#define vtkLine_h

#include "vtkCell.h"
#include "vtkCommonDataModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkLine : public vtkCell
{
public:
  static vtkLine* New();
  vtkTypeMacro(vtkLine, vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() override { return VTK_LINE; }
  int GetCellDimension() override { return 1; }
  int GetNumberOfEdges() override { return 0; }
  int GetNumberOfFaces() override { return 0; }
  vtkCell* GetEdge(int) override { return nullptr; }
  vtkCell* GetFace(int) override { return nullptr; }
  int CellBoundary(int subId, const double pcoords[3], vtkIdList* pts) override;
  void Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;
  int EvaluatePosition(const double x[3], double closestPoint[3], int& subId, double pcoords[3],
    double& dist2, double weights[]) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights) override;
  int TriangulateLocalIds(int index, vtkIdList* ptIds) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  double* GetParametricCoords() override;
  ///@}

  /**
   * Inflates this line by extending both end by dist. A degenerate line remains
   * untouched.
   *
   * \return 1 if inflation was successful, 0 if no inflation was performed
   */
  int Inflate(double dist) override;

  /**
   * Clip this line using scalar value provided. Like contouring, except
   * that it cuts the line to produce other lines.
   */
  void Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* lines, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut) override;

  /**
   * Return the center of the triangle in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  /**
   * Line-line intersection. Intersection has to occur within [0,1] parametric
   * coordinates and with specified tolerance.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId) override;

  // Return result type for Intersection() and Intersection3D()
  enum IntersectionType
  {
    NoIntersect = 0,
    Intersect = 2,
    OnLine = 3
  };

  // Control the meaning of the provided tolerance.  Fuzzy tolerances allow
  // intersections to occur outside of the range (0<=u,v<=1) as long as they
  // fall within the tolerance provided. Thus non-fuzzy tolerances must be
  // within the (0,1) parametric range (inclusive)
  enum ToleranceType
  {
    Relative = 0,
    Absolute = 1,
    RelativeFuzzy = 2,
    AbsoluteFuzzy = 3
  };

  /**
   * Performs intersection of the projection of two finite 3D lines onto a 2D
   * plane. An intersection is found if the projection of the two lines onto
   * the plane perpendicular to the cross product of the two lines intersect.
   * The parameters (u,v) are the parametric coordinates of the lines at the
   * position of closest approach.
   *
   * The results are of type vtkLine::IntersectionType. An intersection occurs
   * if (u,v) are in the interval [0,1] and the intersection point falls within
   * the tolerance specified. Different types of tolerancing can be used by
   * specifying a tolerance type with the enum provided (vtkLine::ToleranceType).
   * The tolerance types may be: Relative) relative to the projection line lengths
   * (this is default); or Absolute) the distance between the points at (u,v) on
   * the two lines must be less than or equal to the tolerance specified.
   *
   */
  static int Intersection(const double p1[3], const double p2[3], const double x1[3],
    const double x2[3], double& u, double& v, double tolerance = 1e-6,
    int toleranceType = ToleranceType::Relative);

  /**
   * Compute the distance of a point x to a finite line (p1,p2). The method
   * computes the parametric coordinate t and the point location on the
   * line. Note that t is unconstrained (i.e., it may lie outside the range
   * [0,1]) but the closest point will lie within the finite line [p1,p2], if
   * it is defined. Also, the method returns the distance squared between x and
   * the line (p1,p2).
   */
  static double DistanceToLine(const double x[3], const double p1[3], const double p2[3], double& t,
    double closestPoint[3] = nullptr);

  /**
   * Determine the distance of the current vertex to the edge defined by
   * the vertices provided.  Returns distance squared. Note: line is assumed
   * infinite in extent.
   */
  static double DistanceToLine(const double x[3], const double p1[3], const double p2[3]);

  /**
   * Computes the shortest distance squared between two infinite lines, each
   * defined by a pair of points (l0,l1) and (m0,m1).
   * Upon return, the closest points on the two line segments will be stored
   * in closestPt1 and closestPt2. Their parametric coords
   * (-inf <= t0, t1 <= inf) will be stored in t0 and t1. The return value is
   * the shortest distance squared between the two line-segments.
   */
  static double DistanceBetweenLines(double l0[3], double l1[3], double m0[3], double m1[3],
    double closestPt1[3], double closestPt2[3], double& t1, double& t2);

  /**
   * Computes the shortest distance squared between two finite line segments
   * defined by their end points (l0,l1) and (m0,m1).
   * Upon return, the closest points on the two line segments will be stored
   * in closestPt1 and closestPt2. Their parametric coords (0 <= t0, t1 <= 1)
   * will be stored in t0 and t1. The return value is the shortest distance
   * squared between the two line-segments.
   */
  static double DistanceBetweenLineSegments(double l0[3], double l1[3], double m0[3], double m1[3],
    double closestPt1[3], double closestPt2[3], double& t1, double& t2);

  static void InterpolationFunctions(const double pcoords[3], double weights[2]);
  static void InterpolationDerivs(const double pcoords[3], double derivs[2]);
  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[2]) override
  {
    vtkLine::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[2]) override
  {
    vtkLine::InterpolationDerivs(pcoords, derivs);
  }
  ///@}

protected:
  vtkLine();
  ~vtkLine() override = default;

private:
  vtkLine(const vtkLine&) = delete;
  void operator=(const vtkLine&) = delete;
};

//----------------------------------------------------------------------------
inline int vtkLine::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = 0.5;
  pcoords[1] = pcoords[2] = 0.0;
  return 0;
}

VTK_ABI_NAMESPACE_END
#endif
